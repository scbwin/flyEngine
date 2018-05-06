#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <math/MathHelpers.h>
#include <System.h>
#include <renderer/ProjectionParams.h>
#include <math/FlyMath.h>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <Model.h>
#include <memory>
#include <Entity.h>
#include <Camera.h>
#include <Light.h>
#include <iostream>
#include <Quadtree.h>
#include <Octree.h>
#include <Settings.h>
#include <functional>
#include <GraphicsSettings.h>
#include <Timing.h>
#include <SkydomeRenderable.h>

#define RENDERER_STATS 1

namespace fly
{
  template<class API>
  class AbstractRenderer : public System, public GraphicsSettings::Listener
  {
  public:
#if RENDERER_STATS
    struct RendererStats
    {
      unsigned _renderedTriangles;
      unsigned _renderedTrianglesShadow;
      unsigned _renderedMeshes;
      unsigned _renderedMeshesShadow;
      unsigned _bvhTraversalMicroSeconds;
      unsigned _bvhTraversalShadowMapMicroSeconds;
      unsigned _sceneRenderingCPUMicroSeconds;
      unsigned _shadowMapRenderCPUMicroSeconds;
      unsigned _sceneMeshGroupingMicroSeconds;
      unsigned _shadowMapGroupingMicroSeconds;
    };
    const RendererStats& getStats() const { return _stats; }
#endif
    AbstractRenderer(GraphicsSettings const * const gs) : _api(), _gs(gs)
    {
      _pp._near = 0.1f;
      _pp._far = 10000.f;
      _pp._fieldOfViewDegrees = 45.f;
      normalMappingChanged(gs);
      shadowsChanged(gs);
      compositingChanged(gs);
      anisotropyChanged(gs);
      cameraLerpingChanged(gs);
      _gsp._camPosworld = Vec3f(0.f);
    }
    virtual ~AbstractRenderer() {}
    virtual void normalMappingChanged(GraphicsSettings const * const gs) override
    {
      graphicsSettingsChanged();
    }
    virtual void shadowsChanged(GraphicsSettings const * const gs) override
    {
      _shadowMapping = gs->getShadows() || gs->getShadowsPCF();
      graphicsSettingsChanged();
      _shadowMap = _shadowMapping ? _api.createShadowmap(*_gs) : nullptr;
    }
    virtual void shadowMapSizeChanged(GraphicsSettings const * const gs) override
    {
      if (_shadowMap) {
        _api.resizeShadowmap(_shadowMap.get(), *_gs);
      }
    }
    virtual void compositingChanged(GraphicsSettings const * const gs) override
    {
      _offScreenRendering = gs->depthPrepassEnabled() || gs->postProcessingEnabled();
      if (_offScreenRendering) {
        _lightingBuffer = _api.createRenderToTexture(_viewPortSize);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize);
      }
      else {
        _lightingBuffer = nullptr;
        _depthBuffer = nullptr;
      }
      if (gs->postProcessingEnabled()) {
        _api.createCompositeShaderFile(*_gs);
      }
    }
    virtual void windAnimationsChanged(GraphicsSettings const * const gs) override
    {
      graphicsSettingsChanged();
    }
    virtual void cameraLerpingChanged(GraphicsSettings const * const gs) override
    {
      if (gs->getCameraLerping()) {
        _acc = 0.f;
        _cameraLerpAlpha = gs->getCameraLerpAlpha();
      }
    }
    virtual void anisotropyChanged(GraphicsSettings const * const gs) override
    {
      _api.setAnisotropy(gs->getAnisotropy());
    }
    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override
    {
      if (entity->getComponent<fly::StaticMeshRenderable>() == component) {
        auto smr = entity->getComponent<fly::StaticMeshRenderable>();
        _staticMeshRenderables[entity] = smr->hasWind() ?
          std::make_shared<StaticMeshRenderableWind>(smr, _api.createMaterial(smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh())) :
          std::make_shared<StaticMeshRenderable>(smr, _api.createMaterial(smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()));
        _sceneMin = minimum(_sceneMin, smr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, smr->getAABBWorld()->getMax());
      }
      else if (entity->getComponent<fly::DynamicMeshRenderable>() == component) {
        auto dmr = entity->getComponent<fly::DynamicMeshRenderable>();
        _dynamicMeshRenderables[entity] = std::make_shared<DynamicMeshRenderable>(dmr, _api.createMaterial(dmr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(dmr->getMesh()));
      }
      else if (entity->getComponent<Camera>() == component) {
        _camera = entity->getComponent<Camera>();
      }
      else if (entity->getComponent<DirectionalLight>() == component) {
        _directionalLight = entity->getComponent<DirectionalLight>();
      }
      else if (entity->getComponent<fly::SkydomeRenderable>() == component) {
        auto sdr = entity->getComponent<fly::SkydomeRenderable>();
        _skydomeRenderable = std::make_shared<SkydomeRenderable>(_meshGeometryStorage.addMesh(sdr->getMesh()), _api.getSkyboxShaderDesc().get());
      }
    }
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override
    {
      if (entity->getComponent<fly::StaticMeshRenderable>() == component) {
        _bvh->removeElement(_staticMeshRenderables[entity].get());
        _staticMeshRenderables.erase(entity);
      }
      else if (entity->getComponent<fly::DynamicMeshRenderable>() == component) {
        _dynamicMeshRenderables.erase(entity);
      }
      else if (entity->getComponent<Camera>() == component) {
        _camera = nullptr;
      }
      else if (entity->getComponent<DirectionalLight>() == component) {
        _directionalLight = nullptr;
      }
      else if (entity->getComponent<fly::SkydomeRenderable>() == component) {
        _skydomeRenderable = nullptr;
      }
    }
    virtual void update(float time, float delta_time) override
    {
#if RENDERER_STATS
      _stats = {};
#endif
      if (_camera && _directionalLight) {
        if (!_bvh) {
          buildBVH();
        }
        _api.beginFrame();
        if (_gs->getCameraLerping()) {
          _acc += delta_time;
          while (_acc >= _dt) {
            _gsp._camPosworld = glm::mix(glm::vec3(_camera->getPosition()), glm::vec3(_gsp._camPosworld), _cameraLerpAlpha);
            _camEulerAngles = glm::eulerAngles(glm::slerp(glm::quat(_camera->getEulerAngles()), glm::quat(_camEulerAngles), _cameraLerpAlpha));
            _acc -= _dt;
          }
        }
        else {
          _gsp._camPosworld = _camera->getPosition();
          _camEulerAngles = _camera->getEulerAngles();
        }
        _gsp._viewMatrix = _camera->getViewMatrix(_gsp._camPosworld, _camEulerAngles);
        _vpScene = _gsp._projectionMatrix * _gsp._viewMatrix;
        _api.setDepthTestEnabled<true>();
        _api.setFaceCullingEnabled<true>();
        _api.setCullMode<API::CullMode::BACK>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        _api.setDepthWriteEnabled<true>();
        _gsp._lightPosWorld = &_directionalLight->_pos;
        _gsp._lightIntensity = &_directionalLight->getIntensity();
        _gsp._time = time;
        _gsp._exposure = _gs->getExposure();
        _meshGeometryStorage.bind();
        if (_shadowMapping) {
          renderShadowMap();
        }
        _api.setViewport(_viewPortSize);
        _gsp._VP = &_vpScene;
#if RENDERER_STATS
        Timing timing;
#endif
        _camera->extractFrustumPlanes(_vpScene);
        std::vector<MeshRenderable*> visible_meshes = _gs->getDetailCulling() ?
          _bvh->getVisibleElementsWithDetailCulling(*_camera) :
          _bvh->getVisibleElements(*_camera);
#if RENDERER_STATS
        _stats._bvhTraversalMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
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
        if (_skydomeRenderable) { // No need to clear color if every pixel is overdrawn anyway
          if (!_gs->depthPrepassEnabled()) { // Clear depth only if no depth pre pass
            _api.clearRendertarget<false, true, false>(Vec4f());
          }
        }
        else {
          _gs->depthPrepassEnabled() ? _api.clearRendertarget<true, false, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f)) : _api.clearRendertarget<true, true, false>(Vec4f(0.149f, 0.509f, 0.929f, 1.f));
        }
        if (_shadowMap) {
          _api.bindShadowmap(*_shadowMap);
        }
#if RENDERER_STATS
        timing.start();
#endif
        renderMeshes(visible_meshes);
        if (_skydomeRenderable) {
          _api.setCullMode<API::CullMode::FRONT>();
          Mat4f view_matrix_sky_dome = _gsp._viewMatrix;
          view_matrix_sky_dome[3] = Vec4f(Vec3f(0.f), 1.f);
          auto skydome_vp = _gsp._projectionMatrix * view_matrix_sky_dome;
          _gsp._VP = &skydome_vp;
          _api.setupShaderDesc(*_skydomeRenderable->_shaderDesc, _gsp);
          _skydomeRenderable->render(_api);
          _gsp._VP = &_vpScene; // Restore view projection matrix
          _api.setCullMode<API::CullMode::BACK>();
        }
#if RENDERER_STATS
        _stats._sceneRenderingCPUMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
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
      _gsp._projectionMatrix = MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, _viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._far, _api.getZNearMapping());
      compositingChanged(_gs);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    API* getApi() { return &_api; }
    const Vec3f& getSceneMin() const { return _sceneMin; }
    const Vec3f& getSceneMax() const { return _sceneMax; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
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
    GraphicsSettings const * const _gs;
    std::unique_ptr<typename API::RTT> _lightingBuffer;
    std::unique_ptr<typename API::Depthbuffer> _depthBuffer;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    unsigned _defaultRenderTarget = 0;
    Mat4f _vpScene;
    bool _offScreenRendering;
    bool _shadowMapping;
    float _cameraLerpAlpha;
    float _acc = 0.f;
    float _dt = 1.f / 30.f;

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
    struct SkydomeRenderable : public MeshRenderable
    {
      SkydomeRenderable(const typename API::MeshGeometryStorage::MeshData& mesh_data, typename API::ShaderDesc* shader_desc) :
        MeshRenderable(nullptr, mesh_data)
      {
        _shaderDesc = shader_desc;
      }
      virtual AABB* getAABBWorld() const
      {
        return nullptr;
      }
      virtual void render(const API& api)
      {
        api.renderMesh(_meshData);
      }
      virtual void renderDepth(const API& api)
      {}
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
    std::shared_ptr<SkydomeRenderable> _skydomeRenderable;
    using BVH = Quadtree<MeshRenderable>;
    std::unique_ptr<BVH> _bvh;
    void renderQuadtreeAABBs()
    {
      auto visible_nodes = _gs->getDetailCulling()
        ? _bvh->getVisibleNodesWithDetailCulling(*_camera)
        : _bvh->getVisibleNodes(*_camera);
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
#if RENDERER_STATS
      Timing timing;
#endif
      std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<MeshRenderable*>>> display_list;
      for (const auto& e : meshes) {
        display_list[e->_shaderDesc][e->_materialDesc.get()].push_back(e);
      }
#if RENDERER_STATS
      _stats._sceneMeshGroupingMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
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
      auto vp_shadow_volume = _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._fieldOfViewDegrees,
        inverse(_gsp._viewMatrix), _directionalLight->getViewMatrix(), static_cast<float>(_gs->getShadowMapSize()), _gs->getFrustumSplits(), _gsp._worldToLight, _api.getZNearMapping());
#if RENDERER_STATS
      Timing timing;
#endif
      _camera->extractFrustumPlanes(vp_shadow_volume);
      auto visible_meshes = _gs->getDetailCulling() ?
        _bvh->getVisibleElementsWithDetailCulling(*_camera) :
        _bvh->getVisibleElements(*_camera);
#if RENDERER_STATS
      _stats._bvhTraversalShadowMapMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      for (const auto& e : _dynamicMeshRenderables) {
        visible_meshes.push_back(e.second.get());
      }
#if RENDERER_STATS
      timing.start();
#endif
      std::map<typename API::ShaderDesc*, std::map<typename API::MaterialDesc*, std::vector<MeshRenderable*>>> sm_display_list;
      for (const auto& e : visible_meshes) {
        sm_display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
      }
#if RENDERER_STATS
      _stats._shadowMapGroupingMicroSeconds = timing.duration<std::chrono::microseconds>();
      timing.start();
#endif
      _api.setDepthClampEnabled<true>();
      _api.setViewport(Vec2u(_gs->getShadowMapSize()));
      for (unsigned i = 0; i < _gs->getFrustumSplits().size(); i++) {
        _api.setRendertargets({}, _shadowMap.get(), i);
        _api.clearRendertarget<false, true, false>(Vec4f());
        _gsp._VP = &_gsp._worldToLight[i];
        for (const auto& e : sm_display_list) {
          _api.setupShaderDesc(*e.first, _gsp);
          for (const auto& e1 : e.second) {
            e1.first->setupDepth(e.first->getShader().get());
            for (const auto& smr : e1.second) {
              smr->renderDepth(_api);
#if RENDERER_STATS
              _stats._renderedTrianglesShadow += smr->_meshData.numTriangles();
              _stats._renderedMeshesShadow++;
#endif
            }
          }
        }
      }
#if RENDERER_STATS
      _stats._shadowMapRenderCPUMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      _gsp._smFrustumSplits = &_gs->getFrustumSplits();
      _gsp._smBias = _gs->getShadowBias();
      _api.setDepthClampEnabled<false>();
    }
    void graphicsSettingsChanged()
    {
      _api.recreateShadersAndMaterials(*_gs);
      for (const auto& e : _staticMeshRenderables) {
        e.second->fetchShaderDescs();
      }
      for (const auto& e : _dynamicMeshRenderables) {
        e.second->fetchShaderDescs();
      }
    }
    void buildBVH()
    {
      _bvh = std::make_unique<BVH>(_sceneMin, _sceneMax);
      for (const auto& e : _staticMeshRenderables) {
        _bvh->insert(e.second.get());
      }
    }
  };
}

#endif
