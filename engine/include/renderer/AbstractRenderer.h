#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <System.h>
#include <renderer/ProjectionParams.h>
#include <renderer/RenderParams.h>
#include <math/FlyMath.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <Model.h>
#include <memory>
#include <Entity.h>
#include <Camera.h>
#include <Light.h>
#include <iostream>
#include <Quadtree.h>
#include <Settings.h>
#include <functional>
#include <GraphicsSettings.h>
#include <Timing.h>

#define RENDERER_STATS 1

namespace fly
{
  template<class API>
  class AbstractRenderer : public System, public GraphicsSettings::Listener
  {
  public:
    AbstractRenderer(const GraphicsSettings* gs) : _api(), _gs(gs)
    {
      _pp._near = 0.1f;
      _pp._far = 10000.f;
      _pp._fieldOfViewDegrees = 45.f;
      normalMappingChanged(gs->getNormalMapping(), gs->getParallaxMapping(), gs->getReliefMapping());
      shadowsChanged(gs->getShadows(), gs->getShadowsPCF(), gs->getShadowBias(), gs->getFrustumSplits());
      compositingChanged(gs->exposureEnabled(), gs->depthPrepassEnabled(), gs->postProcessingEnabled());
      anisotropyChanged(gs->getAnisotropy());
      cameraLerpingChanged(gs->getCameraLerping(), gs->getCameraLerpAlpha());
      _gsp._camPosworld = Vec3f(0.f);
    }
    virtual ~AbstractRenderer() {}
    virtual void normalMappingChanged(bool normal_mapping, bool parallax_mapping, bool relief_mapping) override
    {
      graphicsSettingsChanged();
    }
    virtual void shadowsChanged(bool shadows, bool shadows_pcf, float bias, const std::vector<float>& frustum_splits) override
    {
      _shadowMapping = shadows || shadows_pcf;
      graphicsSettingsChanged();
      shadowMapSizeChanged(_gs->getShadowMapSize());
    }
    virtual void shadowMapSizeChanged(unsigned size) override
    {
      _shadowMap = _shadowMapping ? _api.createShadowmap(size, *_gs) : nullptr;
    }
    virtual void compositingChanged(bool exposure_enabled, bool depth_pre_pass, bool post_processing) override
    {
      _offScreenRendering = depth_pre_pass || post_processing;
      if (_offScreenRendering) {
        _lightingBuffer = _api.createRenderToTexture(_viewPortSize);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize);
      }
      else {
        _lightingBuffer = nullptr;
        _depthBuffer = nullptr;
      }
      if (post_processing) {
        _api.createCompositeShaderFile(*_gs);
      }
    }
    virtual void windAnimationsChanged(bool wind_animations) override
    {
      graphicsSettingsChanged();
    }
    virtual void cameraLerpingChanged(bool enabled, float alpha) override
    {
      _cameraLerpAlpha = enabled ? alpha : 0.f;
    }
    virtual void anisotropyChanged(unsigned anisotropy) override
    {
      _api.setAnisotropy(anisotropy);
    }
    virtual void onComponentsChanged(Entity* entity) override
    {
      auto camera = entity->getComponent<Camera>();
      auto dl = entity->getComponent<DirectionalLight>();
      auto mr = entity->getComponent<fly::StaticMeshRenderable>();
      auto dmr = entity->getComponent<fly::DynamicMeshRenderable>();
      if (mr) {
        _staticMeshRenderables[entity] = mr->hasWind() ? 
          std::make_shared<StaticMeshRenderableWind>(mr, _api.createMaterial(mr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(mr->getMesh())) :
          std::make_shared<StaticMeshRenderable>(mr, _api.createMaterial(mr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(mr->getMesh()));
        _sceneMin = minimum(_sceneMin, mr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, mr->getAABBWorld()->getMax());
      }
      else {
        auto it = _staticMeshRenderables.find(entity);
        if (it != _staticMeshRenderables.end()) {
          _quadtree->removeElement(it->second.get());
          _staticMeshRenderables.erase(it->first);
        }
      }
      if (dmr) {
        _dynamicMeshRenderables[entity] = std::make_shared<DynamicMeshRenderable>(dmr, _api.createMaterial(dmr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(dmr->getMesh()));
      }
      else {
        _dynamicMeshRenderables.erase(entity);
      }
      if (camera) {
        _camera = camera;
      }
      if (dl) {
        _directionalLight = dl;
      }
    }
    virtual void update(float time, float delta_time) override
    {
#if RENDERER_STATS
      _stats = {};
#endif
      if (_camera && _directionalLight) {
        if (!_quadtree) {
          buildQuadtree();
        }
        _api.beginFrame();
        _gsp._camPosworld = glm::mix(_camera->_pos, glm::vec3(_gsp._camPosworld), _cameraLerpAlpha);
        _camEulerAngles = glm::eulerAngles(glm::slerp(glm::quat(_camera->_eulerAngles), glm::quat(_camEulerAngles), _cameraLerpAlpha));
        _gsp._viewMatrix = _camera->getViewMatrix(_gsp._camPosworld, _camEulerAngles);
        _vpScene = _gsp._projectionMatrix * _gsp._viewMatrix;
        _api.setDepthTestEnabled<true>();
        _api.setFaceCullingEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        _api.setDepthWriteEnabled<true>();
        _gsp._lightPosWorld = _directionalLight->_pos;
        _gsp._lightIntensity = _directionalLight->getIntensity();
        _gsp._time = time;
        _gsp._exposure = _gs->getExposure();
        _meshGeometryStorage.bind();
        if (_shadowMapping) {
          renderShadowMap();
        }
        _api.setViewport(_viewPortSize);
        _gsp._VP = &_vpScene;
        std::vector<MeshRenderable*> visible_meshes = _gs->getDetailCulling() ?
          _quadtree->getVisibleElementsWithDetailCulling<API::isDirectX()>(_vpScene, _gsp._camPosworld) :
          _quadtree->getVisibleElements<API::isDirectX()>(_vpScene);
        for (const auto& e : _dynamicMeshRenderables) {
          visible_meshes.push_back(e.second.get());
        }
        if (_gs->depthPrepassEnabled()) {
          _api.setRendertargets({}, _depthBuffer.get());
          _api.clearRendertarget<false, true, false>(Vec4f());
          std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<MeshRenderable*>>> display_list;
          for (const auto& e : visible_meshes) {
            display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShaderDesc(*e.first, _gsp);
            for (const auto& e1 : e.second) {
              e1.first->setupDepth(e.first->getShader().get());
              for (const auto& smr : e1.second) {
                smr->renderDepth(_api);
              }
            }
          }
          _api.setDepthWriteEnabled<false>();
          _api.setDepthFunc<API::DepthFunc::EQUAL>();
        }
        _offScreenRendering ? _api.setRendertargets({ _lightingBuffer.get() }, _depthBuffer.get()) : _api.bindBackbuffer(_defaultRenderTarget);
        _gs->depthPrepassEnabled() ? _api.clearRendertarget<true, false, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f)) : _api.clearRendertarget<true, true, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f));
        if (_shadowMap) {
          _api.bindShadowmap(*_shadowMap);
        }
        renderMeshes(visible_meshes);
        if (_gs->getDebugQuadtreeNodeAABBs()) {
          renderQuadtreeAABBs();
        }
        if (_gs->getDebugObjectAABBs()) {
          renderObjectAABBs(visible_meshes);
        }
        if (_offScreenRendering) {
          _api.setDepthTestEnabled<false>();
          _api.bindBackbuffer(_defaultRenderTarget);
          _api.composite(_lightingBuffer.get(), _gsp);
        }
        _api.endFrame();
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      float aspect_ratio = _viewPortSize[0] / _viewPortSize[1];
      _gsp._projectionMatrix = _api.getZNearMapping() == ZNearMapping::ZERO ?
        glm::perspectiveRH_ZO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far) :
        glm::perspectiveRH_NO(glm::radians(_pp._fieldOfViewDegrees), aspect_ratio, _pp._near, _pp._far);

      compositingChanged(_gs->exposureEnabled(), _gs->depthPrepassEnabled(), _gs->postProcessingEnabled());
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    API* getApi() { return &_api; }
    const Vec3f& getSceneMin() const { return _sceneMin; }
    const Vec3f& getSceneMax() const { return _sceneMax; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
#if RENDERER_STATS
    struct RendererStats
    {
      unsigned _renderedTriangles;
      unsigned _renderedTrianglesShadow;
      unsigned _renderedMeshes;
      unsigned _renderedMeshesShadow;
    };
    const RendererStats& getStats() const { return _stats; }
#endif
  private:
    API _api;
    ProjectionParams _pp;
    GlobalShaderParams _gsp;
    Vec2f _viewPortSize = Vec2f(1.f);
    Vec3f _camEulerAngles = Vec3f(0.f);
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    Vec3f _sceneMin = Vec3f(std::numeric_limits<float>::max());
    Vec3f _sceneMax = Vec3f(std::numeric_limits<float>::lowest());
    const GraphicsSettings* _gs;
    std::unique_ptr<typename API::RTT> _lightingBuffer;
    std::unique_ptr<typename API::Depthbuffer> _depthBuffer;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    unsigned _defaultRenderTarget = 0;
    Mat4f _vpScene;
    bool _offScreenRendering;
    bool _shadowMapping;
    float _cameraLerpAlpha;

#if RENDERER_STATS
    RendererStats _stats;
#endif

    struct MeshRenderable
    {
      std::shared_ptr<typename API::MaterialDesc> _materialDesc;
      typename API::MeshGeometryStorage::MeshData _meshData;
      typename API::ShaderDesc* _shaderDesc;
      typename API::ShaderDesc* _shaderDescDepth;
      virtual void render(const API& api) = 0;
      virtual void renderDepth(const API& api) = 0;
      MeshRenderable(const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) : 
      _materialDesc(material_desc), _meshData(mesh_data)
      {}
      virtual void fetchShaderDescs()
      {
        _shaderDesc = _materialDesc->getMeshShaderDesc(false).get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepth(false).get();
      }
      virtual AABB* getAABBWorld() const = 0;
    };

    struct DynamicMeshRenderable : public MeshRenderable
    {
      DynamicMeshRenderable(const std::shared_ptr<fly::DynamicMeshRenderable>& dmr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
        MeshRenderable(material_desc, mesh_data),
        _dmr(dmr)
      {
        fetchShaderDescs();
      }
      std::shared_ptr<fly::DynamicMeshRenderable> _dmr;
      virtual void render(const API& api) override
      {
        const auto& model_matrix = _dmr->getModelMatrix();
        api.renderMesh(_meshData, model_matrix, _dmr->getModelMatrixInverse());
      }
      virtual void renderDepth(const API& api) override
      {
        api.renderMesh(_meshData, _dmr->getModelMatrix());
      }
      virtual AABB* getAABBWorld() const override { return _dmr->getAABBWorld(); }
    };
    struct StaticMeshRenderable : public MeshRenderable
    {
      std::shared_ptr<fly::StaticMeshRenderable> _smr;
      StaticMeshRenderable(const std::shared_ptr<fly::StaticMeshRenderable>& smr, 
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
        MeshRenderable(material_desc, mesh_data),
        _smr(smr)
      {
        fetchShaderDescs();
      }
      virtual ~StaticMeshRenderable() = default;
      virtual void render(const API& api) override
      {
        api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse());
      }
      virtual void renderDepth(const API& api) override
      {
        api.renderMesh(_meshData, _smr->getModelMatrix());
      }
      virtual AABB* getAABBWorld() const override { return _smr->getAABBWorld(); }
    };
    struct StaticMeshRenderableWind : public StaticMeshRenderable
    {
      StaticMeshRenderableWind(const std::shared_ptr<fly::StaticMeshRenderable>& smr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
        StaticMeshRenderable(smr, material_desc, mesh_data)
      {
        fetchShaderDescs();
      }
      virtual ~StaticMeshRenderableWind() = default;
      virtual void fetchShaderDescs() override
      {
        _shaderDesc = _materialDesc->getMeshShaderDesc(true).get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepth(true).get();
      }
      virtual void render(const API& api) override
      {
        api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getWindParams(), *getAABBWorld());
      }
      virtual void renderDepth(const API& api) override
      {
        api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getWindParams(), *getAABBWorld());
      }
    };
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::map<Entity*, std::shared_ptr<StaticMeshRenderable>> _staticMeshRenderables;
    std::map<Entity*, std::shared_ptr<DynamicMeshRenderable>> _dynamicMeshRenderables;
    std::unique_ptr<Quadtree<MeshRenderable>> _quadtree;
    void renderQuadtreeAABBs()
    {
      auto visible_nodes = _quadtree->getVisibleNodes(_vpScene);
      if (visible_nodes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB*> aabbs;
        for (const auto& n : visible_nodes) {
          aabbs.push_back(n->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(1.f, 0.f, 0.f));
      }
    }
    void renderObjectAABBs(const std::vector<MeshRenderable*>& visible_elements)
    {
      if (visible_elements.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB*> aabbs;
        for (const auto& e : visible_elements) {
          aabbs.push_back(e->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(0.f, 1.f, 0.f));
      }
    }
    void renderMeshes(const std::vector<MeshRenderable*>& meshes)
    {
      std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<MeshRenderable*>>> display_list;
      for (const auto& e : meshes) {
        display_list[e->_shaderDesc][e->_materialDesc.get()].push_back(e);
      }
      for (const auto& e : display_list) {
        _api.setupShaderDesc(*e.first, _gsp);
        for (const auto& e1 : e.second) {
          e1.first->setup(e.first->getShader().get());
          for (const auto& smr : e1.second) {
            smr->render(_api);
#if RENDERER_STATS
            _stats._renderedTriangles += smr->_meshData.numTriangles();
            _stats._renderedMeshes++;
#endif
          }
        }
      }
    }
    void renderShadowMap()
    {
      std::vector<Mat4f> light_vps;
      auto vp_shadow_volume = _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._fieldOfViewDegrees,
        inverse(_gsp._viewMatrix), _directionalLight->getViewMatrix(), static_cast<float>(_gs->getShadowMapSize()), _gs->getFrustumSplits(), light_vps, _api.isDirectX());
      auto visible_meshes = _gs->getDetailCulling() ?
        _quadtree->getVisibleElementsWithDetailCulling<API::isDirectX()>(vp_shadow_volume, _gsp._camPosworld) :
        _quadtree->getVisibleElements<API::isDirectX()>(vp_shadow_volume);
      for (const auto& e : _dynamicMeshRenderables) {
        visible_meshes.push_back(e.second.get());
      }
      std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<MeshRenderable*>>> sm_display_list;
      for (const auto& e : visible_meshes) {
        sm_display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
      }
      _api.setDepthClampEnabled<true>();
      _api.setViewport(Vec2u(_gs->getShadowMapSize()));
      for (unsigned i = 0; i < _gs->getFrustumSplits().size(); i++) {
        _api.setRendertargets({}, _shadowMap.get(), i);
        _api.clearRendertarget<false, true, false>(Vec4f());
        _gsp._VP = &light_vps[i];
        for (const auto& e : sm_display_list) {
          _api.setupShaderDesc(*e.first, _gsp);
          for (const auto& e1 : e.second) {
            e1.first->setupDepth(e.first->getShader().get());
            for (const auto& smr : e1.second) {
              smr->renderDepth(_api);
#if RENDERER_STATS
              _stats._renderedTrianglesShadow += smr->_meshData.numTriangles();
              _stats._renderedMeshesShadow ++;
#endif
            }
          }
        }
      }
      _gsp._worldToLight = light_vps;
      _gsp._smFrustumSplits = _gs->getFrustumSplits();
      _gsp._smBias = _gs->getShadowBias();
      _api.setDepthClampEnabled<false>();
    }
    void graphicsSettingsChanged()
    {
      _api.recreateShadersAndMaterials(*_gs);
      for (const auto& e : _staticMeshRenderables) {
        e.second->fetchShaderDescs();
      }
    }
    void buildQuadtree()
    {
      _quadtree = std::make_unique<Quadtree<MeshRenderable>>(_sceneMin, _sceneMax);
      for (const auto& e : _staticMeshRenderables) {
        _quadtree->insert(e.second.get());
      }
    }
  };
}

#endif
