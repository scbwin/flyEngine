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
      screenSpaceReflectionsChanged(gs);
      depthOfFieldChanged(gs);
    //  compositingChanged(gs);
      anisotropyChanged(gs);
      cameraLerpingChanged(gs);
      _gsp._camPosworld = Vec3f(0.f);
    }
    virtual ~AbstractRenderer() {}
    virtual void normalMappingChanged(GraphicsSettings const * gs) override
    {
      graphicsSettingsChanged();
    }
    virtual void depthOfFieldChanged(GraphicsSettings const * gs) override
    {
      if (gs->getDepthOfField()) {
        for (unsigned i = 0; i < _dofBuffer.size(); i++) {
          _dofBuffer[i] = _api.createRenderToTexture(_viewPortSize * gs->getDepthOfFieldScaleFactor(), API::TexFilter::LINEAR);
        }
        _api.createBlurShader(*gs);
      }
      else {
        for (unsigned i = 0; i < _dofBuffer.size(); i++) {
          _dofBuffer[i] = nullptr;
        }
      }
      compositingChanged(gs);
    }
    virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) override
    {
      if (gs->getScreenSpaceReflections()) {
        _api.createScreenSpaceReflectionsShader(*gs);
      }
      graphicsSettingsChanged();
      compositingChanged(gs);
    }
    virtual void shadowsChanged(GraphicsSettings const * gs) override
    {
      _shadowMapping = gs->getShadows() || gs->getShadowsPCF();
      graphicsSettingsChanged();
      _shadowMap = _shadowMapping ? _api.createShadowmap(*_gs) : nullptr;
    }
    virtual void shadowMapSizeChanged(GraphicsSettings const * gs) override
    {
      if (_shadowMap) {
        _api.resizeShadowmap(_shadowMap.get(), *_gs);
      }
    }
    virtual void compositingChanged(GraphicsSettings const * gs) override
    {
      _offScreenRendering = gs->depthPrepassEnabled() || gs->postProcessingEnabled();
      if (_offScreenRendering) {
        _lightingBuffer = _api.createRenderToTexture(_viewPortSize, API::TexFilter::NEAREST);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize, gs->getScreenSpaceReflections());
      }
      else {
        _lightingBuffer = nullptr;
        _depthBuffer = nullptr;
      }
      _viewSpaceNormals = gs->getScreenSpaceReflections() ? _api.createRenderToTexture(_viewPortSize, API::TexFilter::NEAREST) : nullptr;
      if (gs->postProcessingEnabled()) {
        _api.createCompositeShader(*_gs);
      }
    }
    virtual void windAnimationsChanged(GraphicsSettings const * gs) override
    {
      graphicsSettingsChanged();
    }
    virtual void gammaChanged(GraphicsSettings const * gs) override
    {
      graphicsSettingsChanged();
      compositingChanged(gs);
    }
    virtual void cameraLerpingChanged(GraphicsSettings const * gs) override
    {
      if (gs->getCameraLerping()) {
        _acc = 0.f;
        _cameraLerpAlpha = gs->getCameraLerpAlpha();
      }
    }
    virtual void anisotropyChanged(GraphicsSettings const * gs) override
    {
      _api.setAnisotropy(gs->getAnisotropy());
    }
    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override
    {
      if (entity->getComponent<fly::StaticMeshRenderable>() == component) {
        auto smr = entity->getComponent<fly::StaticMeshRenderable>();
        if (smr->hasWind()) {
          _staticMeshRenderables[entity] = std::make_shared<StaticMeshRenderableWind>(smr, _api.createMaterial(smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), *this);
        }
        else if (smr->getMaterial()->isReflective()) {
          _staticMeshRenderables[entity] = std::make_shared<StaticMeshRenderableReflective>(smr, _api.createMaterial(smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), *this);
        }
        else {
          _staticMeshRenderables[entity] = std::make_shared<StaticMeshRenderable>(smr, _api.createMaterial(smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), *this);
        }
        _sceneMin = minimum(_sceneMin, smr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, smr->getAABBWorld()->getMax());
      }
      else if (entity->getComponent<fly::DynamicMeshRenderable>() == component) {
        auto dmr = entity->getComponent<fly::DynamicMeshRenderable>();
        _dynamicMeshRenderables[entity] = std::make_shared<DynamicMeshRenderable>(dmr, _api.createMaterial(dmr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(dmr->getMesh()), *this);
      }
      else if (entity->getComponent<Camera>() == component) {
        _camera = entity->getComponent<Camera>();
      }
      else if (entity->getComponent<DirectionalLight>() == component) {
        _directionalLight = entity->getComponent<DirectionalLight>();
      }
      else if (entity->getComponent<fly::SkydomeRenderable>() == component) {
        auto sdr = entity->getComponent<fly::SkydomeRenderable>();
        _skydomeRenderable = std::make_shared<SkydomeRenderable>(_meshGeometryStorage.addMesh(sdr->getMesh()), _api.getSkyboxShaderDesc().get(), *this);
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
        _viewMatrixInverse = inverse(glm::mat3(_gsp._viewMatrix));
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
        _gsp._gamma = _gs->getGamma();
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
          if (_camera->intersectFrustumAABB(*e.second->getAABBWorld()) != IntersectionResult::OUTSIDE) {
            visible_meshes.push_back(e.second.get());
          }
        }
        if (_gs->depthPrepassEnabled()) {
          _api.setRendertargets({}, _depthBuffer.get());
          _api.clearRendertarget<false, true, false>(Vec4f());
          std::map<typename API::ShaderDesc const *, std::map<typename API::MaterialDesc const *, std::vector<MeshRenderable*>>> display_list;
          for (const auto& e : visible_meshes) {
            display_list[e->_shaderDescDepth][e->_materialDesc.get()].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShaderDesc(*e.first, _gsp);
            for (const auto& e1 : e.second) {
              e1.first->setupDepth();
              for (const auto& smr : e1.second) {
                smr->renderDepth();
              }
            }
          }
          _api.setDepthWriteEnabled<false>();
          _api.setDepthFunc<API::DepthFunc::EQUAL>();
        }
        //_offScreenRendering ? _api.setRendertargets({ _lightingBuffer.get() }, _depthBuffer.get()) : _api.bindBackbuffer(_defaultRenderTarget);
        if (_offScreenRendering) {
          std::vector<typename API::RTT const *> rendertargets = { _lightingBuffer.get() };
          if (_gs->getScreenSpaceReflections()) {
            rendertargets.push_back(_viewSpaceNormals.get());
          }
          _api.setRendertargets(rendertargets, _depthBuffer.get());
        }
        else {
          _api.bindBackbuffer(_defaultRenderTarget);
        }
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
        renderMeshes(visible_meshes);
        if (_skydomeRenderable) {
          _api.setCullMode<API::CullMode::FRONT>();
          Mat4f view_matrix_sky_dome = _gsp._viewMatrix;
          view_matrix_sky_dome[3] = Vec4f(Vec3f(0.f), 1.f);
          auto skydome_vp = _gsp._projectionMatrix * view_matrix_sky_dome;
          _gsp._VP = &skydome_vp;
          _api.setupShaderDesc(*_skydomeRenderable->_shaderDesc, _gsp);
          _skydomeRenderable->render();
          _gsp._VP = &_vpScene; // Restore view projection matrix
          _api.setCullMode<API::CullMode::BACK>();
        }
        if (_gs->getDebugQuadtreeNodeAABBs()) {
          renderQuadtreeAABBs();
        }
        if (_gs->getDebugObjectAABBs()) {
          renderObjectAABBs(visible_meshes);
        }
        if (_offScreenRendering) {
          _api.setDepthTestEnabled<false>();
        }
        if (_gs->getScreenSpaceReflections()) {
          _api.ssr(*_lightingBuffer, *_viewSpaceNormals, *_depthBuffer, _gsp._projectionMatrix, Vec4f(_gs->getSSRBlendWeight()));
        }
        if (_gs->getDepthOfField()) {
          _api.separableBlur(*_lightingBuffer, _dofBuffer);
          _api.setViewport(_viewPortSize);
        }
        if (_offScreenRendering) {
          _api.bindBackbuffer(_defaultRenderTarget);
          _gs->getDepthOfField() ? _api.composite(*_lightingBuffer, _gsp, *_dofBuffer[0], *_depthBuffer) : _api.composite(*_lightingBuffer, _gsp);
        }
        _api.endFrame();
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      _gsp._projectionMatrix = MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, _viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._far, _api.getZNearMapping());
      depthOfFieldChanged(_gs);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    API* getApi() { return &_api; }
    const Vec3f& getSceneMin() const { return _sceneMin; }
    const Vec3f& getSceneMax() const { return _sceneMax; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
    const Mat4f& getViewProjectionMatrix() const
    {
      return _vpScene;
    }
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
    std::unique_ptr<typename API::RTT> _viewSpaceNormals;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    std::array<std::shared_ptr<typename API::RTT>, 2> _dofBuffer;
    unsigned _defaultRenderTarget = 0;
    Mat4f _vpScene;
    Mat3f _viewMatrixInverse;
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
      std::shared_ptr<typename API::MaterialDesc> const _materialDesc;
      typename API::MeshGeometryStorage::MeshData const _meshData;
      typename API::ShaderDesc const * _shaderDesc;
      typename API::ShaderDesc const * _shaderDescDepth;
      AbstractRenderer const & _renderer;
      virtual void render() = 0;
      virtual void renderDepth() = 0;
      MeshRenderable(const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, AbstractRenderer const & renderer) :
        _materialDesc(material_desc), _meshData(mesh_data), _renderer(renderer)
      {}
      virtual void fetchShaderDescs()
      {
        _shaderDesc = _materialDesc->getMeshShaderDesc().get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepth().get();
      }
      virtual AABB const * getAABBWorld() const = 0;
    };
    struct SkydomeRenderable : public MeshRenderable
    {
      SkydomeRenderable(const typename API::MeshGeometryStorage::MeshData& mesh_data, typename API::ShaderDesc* shader_desc, AbstractRenderer const & renderer) :
        MeshRenderable(nullptr, mesh_data, renderer)
      {
        _shaderDesc = shader_desc;
      }
      virtual AABB const * getAABBWorld() const
      {
        return nullptr;
      }
      virtual void render()
      {
        _renderer._api.renderMesh(_meshData);
      }
      virtual void renderDepth()
      {}
    };
    struct DynamicMeshRenderable : public MeshRenderable
    {
      DynamicMeshRenderable(const std::shared_ptr<fly::DynamicMeshRenderable>& dmr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, AbstractRenderer const & renderer) :
        MeshRenderable(material_desc, mesh_data, renderer),
        _dmr(dmr)
      {
        fetchShaderDescs();
      }
      std::shared_ptr<fly::DynamicMeshRenderable> _dmr;
      virtual void render() override
      {
        const auto& model_matrix = _dmr->getModelMatrix();
        _renderer._api.renderMesh(_meshData, model_matrix, _dmr->getModelMatrixInverse());
      }
      virtual void renderDepth() override
      {
        _renderer._api.renderMesh(_meshData, _dmr->getModelMatrix());
      }
      virtual AABB const * getAABBWorld() const override { return _dmr->getAABBWorld(); }
    };
    struct StaticMeshRenderable : public MeshRenderable
    {
      std::shared_ptr<fly::StaticMeshRenderable> _smr;
      StaticMeshRenderable(const std::shared_ptr<fly::StaticMeshRenderable>& smr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, AbstractRenderer const & renderer) :
        MeshRenderable(material_desc, mesh_data, renderer),
        _smr(smr)
      {
        fetchShaderDescs();
      }
      virtual ~StaticMeshRenderable() = default;
      virtual void render() override
      {
        _renderer._api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse());
      }
      virtual void renderDepth() override
      {
        _renderer._api.renderMesh(_meshData, _smr->getModelMatrix());
      }
      virtual AABB const * getAABBWorld() const override { return _smr->getAABBWorld(); }
    };
    struct StaticMeshRenderableReflective : public StaticMeshRenderable
    {
      StaticMeshRenderableReflective(const std::shared_ptr<fly::StaticMeshRenderable>& smr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, AbstractRenderer const & renderer) :
        StaticMeshRenderable(smr, material_desc, mesh_data, renderer)
      {
        fetchShaderDescs();
      }
      virtual void fetchShaderDescs() override
      {
        _shaderDesc = _materialDesc->getMeshShaderDescReflective().get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepth().get();
      }
      virtual ~StaticMeshRenderableReflective() = default;
      virtual void render() override
      {
        _renderer._api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getModelMatrixInverse() * _renderer._viewMatrixInverse);
      }
    };
    struct StaticMeshRenderableWind : public StaticMeshRenderable
    {
      StaticMeshRenderableWind(const std::shared_ptr<fly::StaticMeshRenderable>& smr,
        const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, AbstractRenderer const & renderer) :
        StaticMeshRenderable(smr, material_desc, mesh_data, renderer)
      {
        fetchShaderDescs();
      }
      virtual ~StaticMeshRenderableWind() = default;
      virtual void fetchShaderDescs() override
      {
        _shaderDesc = _materialDesc->getMeshShaderDescWind().get();
        _shaderDescDepth = _materialDesc->getMeshShaderDescDepthWind().get();
      }
      virtual void render() override
      {
        _renderer._api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getWindParams(), *getAABBWorld());
      }
      virtual void renderDepth() override
      {
        _renderer._api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getWindParams(), *getAABBWorld());
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
        std::vector<AABB const *> aabbs;
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
        std::vector<AABB const *> aabbs;
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
      std::map<typename API::ShaderDesc const *, std::map<typename API::MaterialDesc const *, std::vector<MeshRenderable*>>> display_list;
      for (const auto& e : meshes) {
        display_list[e->_shaderDesc][e->_materialDesc.get()].push_back(e);
      }
#if RENDERER_STATS
      _stats._sceneMeshGroupingMicroSeconds = timing.duration<std::chrono::microseconds>();
      timing.start();
#endif
      for (const auto& e : display_list) {
        _api.setupShaderDesc(*e.first, _gsp);
        for (const auto& e1 : e.second) {
          e1.first->setup();
          for (const auto& smr : e1.second) {
            smr->render();
#if RENDERER_STATS
            _stats._renderedTriangles += smr->_meshData.numTriangles();
            _stats._renderedMeshes++;
#endif
          }
        }
      }
#if RENDERER_STATS
      _stats._sceneRenderingCPUMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
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
      std::map<typename API::ShaderDesc const *, std::map<typename API::MaterialDesc const *, std::vector<MeshRenderable*>>> sm_display_list;
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
            e1.first->setupDepth();
            for (const auto& smr : e1.second) {
              smr->renderDepth();
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
      _gsp._shadowDarkenFactor = _gs->getShadowDarkenFactor();
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
      std::cout << "Static mesh renderables: " << _staticMeshRenderables.size() << std::endl;
      Timing timing;
      for (const auto& e : _staticMeshRenderables) {
        _bvh->insert(e.second.get());
      }
      std::cout << "BVH construction took " << timing.duration<std::chrono::milliseconds>() << " milliseconds." << std::endl;
    }
  };
}

#endif
