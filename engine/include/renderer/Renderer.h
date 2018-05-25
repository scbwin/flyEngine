#ifndef RENDERER_H
#define RENDERER_H

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
#include <StackPOD.h>
#include <Flags.h>
#include <MaterialDesc.h>
#include <SoftwareCache.h>
#include <renderer/MeshRenderables.h>
#include <set>
#include <GameTimer.h>

#define RENDERER_STATS 1

namespace fly
{
  template<class API>
  class Renderer : public System, public GraphicsSettings::Listener
  {
  public:
#if RENDERER_STATS
    struct RendererStats
    {
      unsigned _renderedTriangles;
      unsigned _renderedTrianglesShadow;
      unsigned _renderedMeshes;
      unsigned _renderedMeshesShadow;
      unsigned _cullingMicroSeconds;
      unsigned _cullingShadowMapMicroSeconds;
      unsigned _sceneRenderingCPUMicroSeconds;
      unsigned _shadowMapRenderCPUMicroSeconds;
      unsigned _sceneMeshGroupingMicroSeconds;
      unsigned _shadowMapGroupingMicroSeconds;
      unsigned _rendererTotalCPUMicroSeconds;
    };
    const RendererStats& getStats() const { return _stats; }
#endif
    Renderer(GraphicsSettings * gs) : _api(Vec4f(0.149f, 0.509f, 0.929f, 1.f)), _gs(gs),
      _matDescCache(SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc<API>>, const std::shared_ptr<Material>&, const GraphicsSettings&>(
        [this](const std::shared_ptr<Material>& material, const GraphicsSettings&  settings) {
      return std::make_shared<MaterialDesc<API>>(material, _api, settings, _textureCache, _shaderDescCache, _shaderCache);
    })),
      _shaderDescCache(SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&>(
        [this](const std::shared_ptr<typename API::Shader>& shader, unsigned flags, API& api) {
      return std::make_shared<ShaderDesc<API>>(shader, flags, api);
    })),
      _textureCache(SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&>([this](const std::string& path) {
      return _api.createTexture(path);
    })),
      _shaderCache(SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&,
        typename API::ShaderSource&, typename API::ShaderSource& >([this](typename API::ShaderSource& vs, typename API::ShaderSource& fs, typename API::ShaderSource& gs) {
      return _api.createShader(vs, fs, gs);
    }))
    {
      _pp._near = 0.1f;
      _pp._far = 10000.f;
      _pp._fieldOfViewDegrees = 45.f;
      _renderTargets.reserve(_api._maxRendertargets);
    }
    virtual ~Renderer() {}
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
        _lightingBufferCopy = std::make_unique<typename API::RTT>(*_lightingBuffer);
        _depthBuffer = _api.createDepthbuffer(_viewPortSize);
      }
      else {
        _lightingBuffer = nullptr;
        _lightingBufferCopy = nullptr;
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
      std::shared_ptr<MeshRenderable<API>> mr = nullptr;
      if (entity->getComponent<fly::StaticMeshRenderable>() == component) {
        auto smr = entity->getComponent<fly::StaticMeshRenderable>();
        if (smr->hasWind()) {
          auto smrw = std::make_shared<StaticMeshRenderableWindWrapper<API>>(smr, _matDescCache.getOrCreate(smr->getMaterial(), smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), _api);
          _staticMeshRenderables[entity] = smrw;
          mr = smrw;
        }
        else if (smr->getMaterial()->isReflective()) {
          auto smrr = std::make_shared<StaticMeshRenderableReflectiveWrapper<API>>(smr, _matDescCache.getOrCreate(smr->getMaterial(), smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), _api, _viewMatrixInverse);
          _staticMeshRenderables[entity] = smrr;
          mr = smrr;
        }
        else {
          _staticMeshRenderables[entity] = std::make_shared<StaticMeshRenderableWrapper<API>>(smr, _matDescCache.getOrCreate(smr->getMaterial(), smr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(smr->getMesh()), _api);
          mr = _staticMeshRenderables[entity];
        }
        _aabbStatic = _aabbStatic.getUnion(*smr->getAABBWorld());
      }
      else if (entity->getComponent<fly::StaticInstancedMeshRenderable>() == component) {
        auto simr = entity->getComponent<fly::StaticInstancedMeshRenderable>();
        std::vector<typename API::MeshGeometryStorage::MeshData> mesh_data;
        for (const auto& m : simr->getMeshes()) {
          mesh_data.push_back(_meshGeometryStorage.addMesh(m));
        }
        _staticInstancedMeshRenderables[entity] = std::make_shared<StaticInstancedMeshRenderableWrapper<API>>(simr, _matDescCache.getOrCreate(simr->getMaterial(), simr->getMaterial(), *_gs), mesh_data, _api, _camera);
        mr = _staticInstancedMeshRenderables[entity];
        _aabbStatic = _aabbStatic.getUnion(*_staticInstancedMeshRenderables[entity]->getAABBWorld());
      }
      else if (entity->getComponent<fly::DynamicMeshRenderable>() == component) {
        auto dmr = entity->getComponent<fly::DynamicMeshRenderable>();
        if (dmr->getMaterial()->isReflective()) {
          auto dmrr = std::make_shared<DynamicMeshRenderableReflectiveWrapper<API>>(dmr, _matDescCache.getOrCreate(dmr->getMaterial(), dmr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(dmr->getMesh()), _api, _viewMatrixInverse);
          _dynamicMeshRenderables[entity] = dmrr;
          mr = dmrr;
        }
        else {
          _dynamicMeshRenderables[entity] = std::make_shared<DynamicMeshRenderableWrapper<API>>(dmr, _matDescCache.getOrCreate(dmr->getMaterial(), dmr->getMaterial(), *_gs), _meshGeometryStorage.addMesh(dmr->getMesh()), _api);
          mr = _dynamicMeshRenderables[entity];
        }
      }
      else if (entity->getComponent<Camera>() == component) {
        _camera = entity->getComponent<Camera>();
        _gsp._camPosworld = _camera->getPosition();
        _camEulerAngles = _camera->getEulerAngles();
      }
      else if (entity->getComponent<DirectionalLight>() == component) {
        _directionalLight = entity->getComponent<DirectionalLight>();
      }
      else if (entity->getComponent<fly::SkydomeRenderable>() == component) {
        auto sdr = entity->getComponent<fly::SkydomeRenderable>();
        _skydomeRenderable = std::make_shared<SkydomeRenderableWrapper<API>>(_meshGeometryStorage.addMesh(sdr->getMesh()), _api.getSkyboxShaderDesc().get(), _api);
      }
      if (mr) {
        _gs->addListener(mr);
      }
    }
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override
    {
      if (entity->getComponent<fly::StaticMeshRenderable>() == component) {
        _bvhStatic->removeElement(_staticMeshRenderables[entity].get());
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
    virtual void update() override
    {
#if RENDERER_STATS
      _stats = {};
      Timing timing_total;
#endif
      if (_camera && _directionalLight) {
        if (!_bvhStatic) {
          buildBVH();
        }
        _api.beginFrame();
        if (_gs->getCameraLerping()) {
          _acc += _gameTimer->getDeltaTimeSeconds();
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
        _gsp._lightPosWorld = &_directionalLight->getPosition();
        _gsp._lightIntensity = &_directionalLight->getIntensity();
        _gsp._time = _gameTimer->getTimeSeconds();
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
        cullMeshes(_vpScene);
#if RENDERER_STATS
        _stats._cullingMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
        if (_gs->depthPrepassEnabled()) {
          _renderTargets.clear();
          _api.setRendertargets(_renderTargets, _depthBuffer.get());
          _api.clearRendertarget(false, true, false);
          groupMeshes<true>();
          renderMeshes<true>();
          _api.setDepthWriteEnabled<false>();
          _api.setDepthFunc<API::DepthFunc::EQUAL>();
        }
        if (_offScreenRendering) {
          _renderTargets.clear();
          _renderTargets.push_back(_lightingBuffer.get());
          if (_gs->getScreenSpaceReflections()) {
            _renderTargets.push_back(_viewSpaceNormals.get());
          }
          _api.setRendertargets(_renderTargets, _depthBuffer.get());
        }
        else {
          _api.bindBackbuffer(_defaultRenderTarget);
        }
        _api.clearRendertarget(_skydomeRenderable == nullptr, !_gs->depthPrepassEnabled(), false);
        if (_shadowMap) {
          _api.bindShadowmap(*_shadowMap);
        }
        renderScene();
        if (_skydomeRenderable) {
          _api.setCullMode<API::CullMode::FRONT>();
          Mat4f view_matrix_sky_dome = _gsp._viewMatrix;
          view_matrix_sky_dome[3] = Vec4f(Vec3f(0.f), 1.f);
          auto skydome_vp = _gsp._projectionMatrix * view_matrix_sky_dome;
          _gsp._VP = &skydome_vp;
          _skydomeRenderable->_shaderDesc->setup(_gsp);
          _skydomeRenderable->render();
          _gsp._VP = &_vpScene; // Restore view projection matrix
          _api.setCullMode<API::CullMode::BACK>();
        }
        if (_gs->getDebugQuadtreeNodeAABBs()) {
          renderQuadtreeAABBs();
        }
        if (_gs->getDebugObjectAABBs()) {
          renderObjectAABBs();
        }
        if (_offScreenRendering) {
          _api.setDepthTestEnabled<false>();
        }
        if (_gs->getScreenSpaceReflections()) {
          _renderTargets.clear();
          _renderTargets.push_back(_lightingBuffer.get());
          _api.setRendertargets(_renderTargets, nullptr);
          _api.ssr(*_lightingBuffer, *_viewSpaceNormals, *_depthBuffer, _gsp._projectionMatrix, Vec4f(_gs->getSSRBlendWeight()), *_lightingBufferCopy);
        }
        if (_gs->getDepthOfField()) {
          _api.separableBlur(*_lightingBuffer, _dofBuffer, _renderTargets);
          _api.setViewport(_viewPortSize);
        }
        if (_offScreenRendering) {
          _api.bindBackbuffer(_defaultRenderTarget);
          _gs->getDepthOfField() ? _api.composite(*_lightingBuffer, _gsp, *_dofBuffer[0], *_depthBuffer) : _api.composite(*_lightingBuffer, _gsp);
        }
        _api.endFrame();
      }
      _stats._rendererTotalCPUMicroSeconds = timing_total.duration<std::chrono::microseconds>();
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      _gsp._projectionMatrix = MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, _viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._far, _api.getZNearMapping());
      depthOfFieldChanged(_gs);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    API* getApi() { return &_api; }
    const AABB& getAABBStatic() const { return _aabbStatic; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
    const Mat4f& getViewProjectionMatrix() const
    {
      return _vpScene;
    }
    using BVH = Quadtree<MeshRenderable<API>>;
    const std::unique_ptr<BVH>& getStaticBVH() const
    {
      return _bvhStatic;
    }
  private:
    API _api;
    ProjectionParams _pp;
    GlobalShaderParams _gsp;
    Vec2f _viewPortSize = Vec2f(1.f);
    Vec3f _camEulerAngles = Vec3f(0.f);
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    AABB _aabbStatic;
    GraphicsSettings * const _gs;
    std::unique_ptr<typename API::RTT> _lightingBuffer;
    std::unique_ptr<typename API::RTT> _lightingBufferCopy;
    std::unique_ptr<typename API::Depthbuffer> _depthBuffer;
    std::unique_ptr<typename API::RTT> _viewSpaceNormals;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    std::array<std::shared_ptr<typename API::RTT>, 2> _dofBuffer;
    typename API::RendertargetStack _renderTargets;
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
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::map<Entity*, std::shared_ptr<StaticMeshRenderableWrapper<API>>> _staticMeshRenderables;
    std::map<Entity*, std::shared_ptr<DynamicMeshRenderableWrapper<API>>> _dynamicMeshRenderables;
    std::map<Entity*, std::shared_ptr<StaticInstancedMeshRenderableWrapper<API>>> _staticInstancedMeshRenderables;
    std::shared_ptr<SkydomeRenderableWrapper<API>> _skydomeRenderable;
    StackPOD<MeshRenderable<API>*> _visibleMeshes;
    std::map<ShaderDesc<API> const *, std::map<MaterialDesc<API> const *, StackPOD<MeshRenderable<API>*, 64>>> _displayList;
    std::unique_ptr<BVH> _bvhStatic;
    SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc<API>>, const std::shared_ptr<Material>&, const GraphicsSettings&> _matDescCache;
    SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&> _shaderDescCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&> _shaderCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&> _textureCache;
    void renderQuadtreeAABBs()
    {
      StackPOD<typename BVH::Node*> visible_nodes;
      _bvhStatic->cullVisibleNodes(visible_nodes, *_camera);
      if (visible_nodes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB const *> aabbs;
        aabbs.reserve(visible_nodes.size());
        for (const auto& n : visible_nodes) {
          aabbs.push_back(n->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(1.f, 0.f, 0.f));
      }
    }
    void renderObjectAABBs()
    {
      if (_visibleMeshes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        std::vector<AABB const *> aabbs;
        for (auto m : _visibleMeshes) {
          aabbs.push_back(m->getAABBWorld());
        }
        _api.renderAABBs(aabbs, _vpScene, Vec3f(0.f, 1.f, 0.f));
      }
    }
    void renderScene()
    {
#if RENDERER_STATS
      Timing timing;
#endif
      groupMeshes();
#if RENDERER_STATS
      _stats._sceneMeshGroupingMicroSeconds = timing.duration<std::chrono::microseconds>();
      timing.start();
#endif
      auto stats = renderMeshes();
#if RENDERER_STATS
      _stats._renderedMeshes = stats._renderedMeshes;
      _stats._renderedTriangles = stats._renderedTriangles;
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
      cullMeshes(vp_shadow_volume);
#if RENDERER_STATS
      _stats._cullingShadowMapMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
#if RENDERER_STATS
      timing.start();
#endif
      groupMeshes<true>();
#if RENDERER_STATS
      _stats._shadowMapGroupingMicroSeconds = timing.duration<std::chrono::microseconds>();
      timing.start();
#endif
      _api.setDepthClampEnabled<true>();
      _api.enablePolygonOffset(_gs->getShadowPolygonOffsetFactor(), _gs->getShadowPolygonOffsetUnits());
      _api.setViewport(Vec2u(_gs->getShadowMapSize()));
      _renderTargets.clear();
      for (unsigned i = 0; i < _gs->getFrustumSplits().size(); i++) {
        _api.setRendertargets(_renderTargets, _shadowMap.get(), i);
        _api.clearRendertarget(false, true, false);
        _gsp._VP = &_gsp._worldToLight[i];
        auto stats = renderMeshes<true>();
#if RENDERER_STATS
        _stats._renderedMeshesShadow += stats._renderedMeshes;
        _stats._renderedTrianglesShadow += stats._renderedTriangles;
#endif
      }
      _api.disablePolygonOffset();

#if RENDERER_STATS
      _stats._shadowMapRenderCPUMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      _gsp._smFrustumSplits = &_gs->getFrustumSplits();
      _gsp._smBias = _gs->getShadowBias();
      _gsp._shadowDarkenFactor = _gs->getShadowDarkenFactor();
      _api.setDepthClampEnabled<false>();
    }
    void buildBVH()
    {
      _visibleMeshes.reserve(_staticMeshRenderables.size() + _dynamicMeshRenderables.size() + _staticInstancedMeshRenderables.size());
      _bvhStatic = std::make_unique<BVH>(_aabbStatic);
      std::cout << "Static mesh renderables: " << _staticMeshRenderables.size() << std::endl;
      std::cout << "Static instancd mesh renderables: " << _staticInstancedMeshRenderables.size() << std::endl;
      Timing timing;
      for (const auto& e : _staticMeshRenderables) {
        _bvhStatic->insert(e.second.get());
      }
      for (const auto& e : _staticInstancedMeshRenderables) {
        _bvhStatic->insert(e.second.get());
      }
      std::cout << "BVH construction took " << timing.duration<std::chrono::milliseconds>() << " milliseconds." << std::endl;
    }
    void graphicsSettingsChanged()
    {
      _shaderCache.clear();
      _shaderDescCache.clear();
      for (const auto& e : _matDescCache.getElements()) {
        e->create(_api, *_gs);
      }
      _api.createCompositeShader(*_gs);
    }
    inline void cullMeshes(const Mat4f& view_projection_matrix)
    {
      _camera->extractFrustumPlanes(view_projection_matrix);

      _visibleMeshes.clear();
      if (_staticInstancedMeshRenderables.size()) {
        _api.prepareCulling(_camera->getFrustumPlanes(), _camera->getPosition());
      }
      // Static meshes
      _bvhStatic->cullVisibleElements(*_camera, _visibleMeshes);
      if (_staticInstancedMeshRenderables.size()) {
        _api.endCulling();
      }

      // Dynamic meshes
      for (const auto& e : _dynamicMeshRenderables) {
        if (e.second->getAABBWorld()->isLargeEnough(_camera->getPosition(), _camera->getDetailCullingThreshold())
          && _camera->intersectFrustumAABB(*e.second->getAABBWorld()) != IntersectionResult::OUTSIDE) {
          _visibleMeshes.push_back(e.second.get());
        }
      }
    }
    template<bool depth = false>
    inline void groupMeshes()
    {
      _displayList.clear();
      for (auto m : _visibleMeshes) {
        _displayList[depth ? m->_shaderDescDepth : m->_shaderDesc][m->_materialDesc.get()].push_back_secure(m);
      }
    }
    struct MeshRenderStats
    {
      unsigned _renderedTriangles;
      unsigned _renderedMeshes;
    };
    template<bool depth = false>
    inline MeshRenderStats renderMeshes()
    {
      MeshRenderStats stats = {};
      for (const auto& e : _displayList) { // For each shader
        e.first->setup(_gsp);
        for (const auto& e1 : e.second) { // For each material
          e1.first->setup<depth>();
          for (const auto& mr : e1.second) { // For each mesh renderable
            depth ? mr->renderDepth() : mr->render();
#if RENDERER_STATS
            stats._renderedTriangles += mr->_meshData.numTriangles();
            stats._renderedMeshes++;
#endif
          }
        }
      }
      return stats;
    }
  };
}

#endif
