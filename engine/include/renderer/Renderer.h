#ifndef RENDERER_H
#define RENDERER_H

#include <math/MathHelpers.h>
#include <System.h>
#include <renderer/ProjectionParams.h>
#include <math/FlyMath.h>
#include <glm/gtx/quaternion.hpp>
#include <map>
#include <Model.h>
#include <memory>
#include <Camera.h>
#include <Light.h>
#include <iostream>
#include <Quadtree.h>
#include <Octree.h>
#include <Settings.h>
#include <functional>
#include <GraphicsSettings.h>
#include <Timing.h>
#include <StackPOD.h>
#include <Flags.h>
#include <MaterialDesc.h>
#include <SoftwareCache.h>
#include <renderer/MeshRenderables.h>
#include <set>
#include <GameTimer.h>
#include <GlobalShaderParams.h>
#include <future>
#include <KdTree.h>

#define RENDERER_STATS 1

namespace fly
{
  template<typename API, typename BV>
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
      unsigned _rendererIdleTimeMicroSeconds;
    };
    const RendererStats& getStats() const { return _stats; }
#endif
    Renderer(GraphicsSettings * gs) : _api(Vec4f(0.149f, 0.509f, 0.929f, 1.f)), _gs(gs),
      _matDescCache(SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc<API>>, const std::shared_ptr<Material>&, const GraphicsSettings&>(
        [this, gs](const std::shared_ptr<Material>& material, const GraphicsSettings&  settings) {
      auto desc = std::make_shared<MaterialDesc<API>>(material, _api, settings, _textureCache, _shaderDescCache, _shaderCache);
      gs->addListener(desc);
      return desc;
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
    virtual void gammaChanged(GraphicsSettings const * gs) override
    {
      graphicsSettingsChanged();
      compositingChanged(gs);
    }
    virtual void anisotropyChanged(GraphicsSettings const * gs) override
    {
      _api.setAnisotropy(gs->getAnisotropy());
    }
    virtual void godRaysChanged(GraphicsSettings const * gs) override
    {
      if (gs->getGodRays()) {
        _godRayBuffer = _api.createRenderToTexture(_viewPortSize * gs->getGodRayScale(), API::TexFilter::LINEAR);
        _api.createGodRayShader(*gs);
      }
      else {
        _godRayBuffer = nullptr;
      }
      graphicsSettingsChanged();
      compositingChanged(gs);
    }
    void addStaticMeshRenderable(const std::shared_ptr<StaticMeshRenderable<API, BV>>& smr) {
      _staticMeshRenderables.push_back(smr);
      _sceneBounds = _sceneBounds.getUnion(smr->getBV());
    }
    void addStaticMeshRenderables(const std::vector<std::shared_ptr<StaticMeshRenderable<API, BV>>>& smrs)
    {
      _staticMeshRenderables.insert(_staticMeshRenderables.begin(), smrs.begin(), smrs.end());
      for (const auto& smr : smrs) {
        _sceneBounds = _sceneBounds.getUnion(smr->getBV());
      }
    }
    void addStaticMeshRenderable(const std::shared_ptr<StaticInstancedMeshRenderable<API, BV>>& simr) {
      _staticInstancedMeshRenderables.push_back(simr);
      _sceneBounds = _sceneBounds.getUnion(simr->getBV());
    }
    void addStaticMeshRenderables(const std::vector<std::shared_ptr<StaticInstancedMeshRenderable<API, BV>>>& simrs)
    {
      _staticInstancedMeshRenderables.insert(_staticInstancedMeshRenderables.begin(), simrs.begin(), simrs.end());
      for (const auto& simr : simrs) {
        _sceneBounds = _sceneBounds.getUnion(simr->getBV());
      }
    }
    void setSkydome(const std::shared_ptr<SkydomeRenderable<API, BV>>& sdr)
    {
      _skydomeRenderable = sdr;
    }
    const std::shared_ptr<SkydomeRenderable<API, BV>>& getSkydome() const
    {
      return _skydomeRenderable;
    }
    void setCamera(const std::shared_ptr<Camera>& camera)
    {
      _camera = camera;
    }
    void setDirectionalLight(const std::shared_ptr<DirectionalLight>& dl)
    {
      _directionalLight = dl;
    }
    virtual void update() override
    {
#if RENDERER_STATS
      _stats = {};
      Timing timing_total;
#endif
      assert(_camera && _directionalLight);
      _api.beginFrame();
      _gsp._camPosworld = _camera->getPosition();
      _gsp._viewMatrix = _camera->updateViewMatrix();
      if (_debugCamera) {
        _debugCamera->updateViewMatrix();
      }
      _gsp._viewMatrixInverse = _camera->getViewMatrixInverse();
      _vpScene = _gsp._projectionMatrix * _gsp._viewMatrix;
      _api.setDepthTestEnabled<true>();
      _api.setFaceCullingEnabled<true>();
      _api.setCullMode<API::CullMode::BACK>();
      _api.setDepthFunc<API::DepthFunc::LEQUAL>();
      _api.setDepthWriteEnabled<true>();
      _gsp._lightDirWorld = &_directionalLight->getDirection();
      _gsp._lightIntensity = &_directionalLight->getIntensity();
      _gsp._time = _gameTimer->getTimeSeconds();
      _gsp._exposure = _gs->getExposure();
      _gsp._gamma = _gs->getGamma();
      _meshGeometryStorage.bind();
      std::future<void> async;
      if (_gs->getMultithreadedCulling() && _shadowMapping) {
        async = std::async(std::launch::async, [this]() {
#if RENDERER_STATS
          Timing timing;
#endif
          cullMeshes(_debugCamera ? _gsp._projectionMatrix *
            _debugCamera->getViewMatrix() : _vpScene, _debugCamera ? *_debugCamera : *_camera, _fullyVisibleMeshesAsync, _intersectedMeshesAsync, _visibleMeshesAsync);
#if RENDERER_STATS
          _stats._cullingMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
        });
      }
      if (_shadowMapping) {
        renderShadowMap();
      }
      if (_gs->getMultithreadedCulling() && _shadowMapping) {
#if RENDERER_STATS
        Timing timing;
#endif
        async.get();
#if RENDERER_STATS
        _stats._rendererIdleTimeMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      }
      else {
#if RENDERER_STATS
        Timing timing;
#endif
        cullMeshes(_debugCamera ? _gsp._projectionMatrix *
          _debugCamera->updateViewMatrix() : _vpScene, _debugCamera ? *_debugCamera : *_camera, _fullyVisibleMeshes, _intersectedMeshes, _visibleMeshes);
#if RENDERER_STATS
        _stats._cullingMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      }
      _api.setViewport(_viewPortSize);
      _gsp._VP = &_vpScene;
      if (_gs->depthPrepassEnabled()) {
        _renderTargets.clear();
        _api.setRendertargets(_renderTargets, _depthBuffer.get());
        _api.clearRendertarget(false, true, false);
        groupMeshes<true>((_gs->getMultithreadedCulling() && _shadowMapping) ? _visibleMeshesAsync : _visibleMeshes);
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
      renderScene(_gs->getMultithreadedCulling() && _shadowMapping ? _visibleMeshesAsync : _visibleMeshes);
      if (_skydomeRenderable) {
        _api.setCullMode<API::CullMode::FRONT>();
        Mat4f view_matrix_sky_dome = _gsp._viewMatrix;
        view_matrix_sky_dome[3] = Vec4f(Vec3f(0.f), 1.f);
        auto skydome_vp = _gsp._projectionMatrix * view_matrix_sky_dome;
        _api.renderSkydome(skydome_vp, _skydomeRenderable->getMeshData());
        _api.setCullMode<API::CullMode::BACK>();
      }
      if (_gs->getDebugBVH()) {
        renderBVHNodes(*_camera, _debugCamera ? *_debugCamera : *_camera);
      }
      if (_gs->getDebugObjectBVs()) {
        renderObjectBVs();
      }
      if (_offScreenRendering || _debugCamera) {
        _api.setDepthTestEnabled<false>();
      }
      if (_debugCamera) {
        _api.renderDebugFrustum(_gsp._projectionMatrix * _debugCamera->updateViewMatrix(_debugCamera->getPosition(), _debugCamera->getEulerAngles()), *_gsp._VP);
      }
      if (_gs->getScreenSpaceReflections()) {
        _renderTargets.clear();
        _renderTargets.push_back(_lightingBuffer.get());
        _api.setRendertargets(_renderTargets, nullptr);
        _api.ssr(*_lightingBuffer, *_viewSpaceNormals, *_depthBuffer, _gsp._projectionMatrix, Vec4f(_gs->getSSRBlendWeight()), *_lightingBufferCopy);
      }
      if (_gs->getDepthOfField()) {
        _api.setViewport(_viewPortSize * _gs->getDepthOfFieldScaleFactor());
        _api.separableBlur(*_lightingBuffer, _dofBuffer, _renderTargets);
        _api.setViewport(_viewPortSize);
      }
      Vec3f god_ray_intensity(0.f);
      if (_gs->getGodRays()) {
        auto light_pos_world = _gsp._camPosworld + *_gsp._lightDirWorld * -_pp._far;
        auto light_pos_h = _vpScene * Vec4f(light_pos_world, 1.f);
        auto light_pos_ndc = light_pos_h.xy() / light_pos_h[3];
        if (light_pos_ndc >= -1.f && light_pos_ndc <= 1.f) {
          auto light_pos_uv = light_pos_ndc * 0.5f + 0.5f;
          _api.setViewport(_viewPortSize * _gs->getGodRayScale());
          _renderTargets.clear();
          _renderTargets.push_back(_godRayBuffer.get());
          _api.setRendertargets(_renderTargets, nullptr);
          _api.renderGodRays(*_depthBuffer, *_lightingBuffer, light_pos_uv);
          _api.setViewport(_viewPortSize);
          float min_dist = std::min(light_pos_uv[0], std::min(light_pos_uv[1], std::min(1.f - light_pos_uv[0], 1.f - light_pos_uv[1])));
          god_ray_intensity = *_gsp._lightIntensity * glm::smoothstep(0.f, _gs->getGodRayFadeDist(), min_dist);
        }
      }
      if (_offScreenRendering) {
        _api.bindBackbuffer(_defaultRenderTarget);
        if (_gs->getDepthOfField() && _gs->getGodRays() && god_ray_intensity > Vec3f(0.f)) {
          _api.composite(*_lightingBuffer, _gsp, *_dofBuffer[0], *_depthBuffer, *_godRayBuffer, god_ray_intensity);
        }
        else {
          // TODO: switch to composite shader without god rays
          _gs->getDepthOfField() ? _api.composite(*_lightingBuffer, _gsp, *_dofBuffer[0], *_depthBuffer) : _api.composite(*_lightingBuffer, _gsp);
        }
      }
      _api.endFrame();
      _stats._rendererTotalCPUMicroSeconds = timing_total.duration<std::chrono::microseconds>();
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      _gsp._projectionMatrix = MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, _viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._far, _api.getZNearMapping());
      godRaysChanged(_gs);
      depthOfFieldChanged(_gs);
    }
    inline void setDefaultRendertarget(unsigned rt) { _defaultRenderTarget = rt; }
    API* getApi() { return &_api; }
    std::vector<std::shared_ptr<Material>> getAllMaterials() { return _api.getAllMaterials(); }
    const Mat4f& getViewProjectionMatrix() const
    {
      return _vpScene;
    }
    using BVH = KdTree<IMeshRenderable<API, BV>, BV>;
    const std::unique_ptr<BVH>& getStaticBVH() const
    {
      return _bvhStatic;
    }
    const BV& getSceneBounds() const
    {
      return _sceneBounds;
    }
    const std::shared_ptr<MaterialDesc<API>> & createMaterialDesc(const std::shared_ptr<Material>& material)
    {
      return _matDescCache.getOrCreate(material, material, *_gs);
    }
    typename API::MeshData addMesh(const std::shared_ptr<Mesh>& mesh)
    {
      return _meshGeometryStorage.addMesh(mesh);
    }
    void setDebugCamera(const std::shared_ptr<Camera>& camera)
    {
      _debugCamera = camera;
    }
    const std::shared_ptr<Camera>& getDebugCamera() const
    {
      return _debugCamera;
    }
    void buildBVH()
    {
      _visibleMeshes.reserve(_staticMeshRenderables.size() + _staticInstancedMeshRenderables.size());
      _visibleMeshesAsync = _visibleMeshes;
      _fullyVisibleMeshes = _visibleMeshes;
      _fullyVisibleMeshesAsync = _visibleMeshes;
      _intersectedMeshes = _visibleMeshes;
      _intersectedMeshesAsync = _visibleMeshes;
      std::cout << "Static mesh renderables: " << _staticMeshRenderables.size() << std::endl;
      std::cout << "Static instanced mesh renderables: " << _staticInstancedMeshRenderables.size() << std::endl;
      if (_visibleMeshes.capacity() == 0) {
        throw std::exception("No meshes were added to the renderer.");
      }
      Timing timing;
      std::vector<IMeshRenderable<API, BV>*> renderables;
      renderables.reserve(_staticMeshRenderables.size() + _staticInstancedMeshRenderables.size());
      for (const auto& smr : _staticMeshRenderables) {
        renderables.push_back(smr.get());
      }
      for (const auto& simr : _staticInstancedMeshRenderables) {
        renderables.push_back(simr.get());
      }
      _bvhStatic = std::make_unique<BVH>(renderables);
      std::cout << "BVH construction took " << timing.duration<std::chrono::milliseconds>() << " milliseconds." << std::endl;
      std::cout << "BVH takes " << static_cast<float>(_bvhStatic->getSizeInBytes()) / 1024.f / 1024.f << " MB memory" << std::endl;
      std::cout << "Scene bounds:" << _bvhStatic->getBV() << std::endl;
    }
  private:
    API _api;
    ProjectionParams _pp;
    GlobalShaderParams _gsp;
    Vec2f _viewPortSize = Vec2f(1.f);
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    std::shared_ptr<Camera> _debugCamera;
    GraphicsSettings * const _gs;
    std::unique_ptr<typename API::RTT> _lightingBuffer;
    std::unique_ptr<typename API::RTT> _lightingBufferCopy;
    std::unique_ptr<typename API::Depthbuffer> _depthBuffer;
    std::unique_ptr<typename API::RTT> _viewSpaceNormals;
    std::unique_ptr<typename API::Shadowmap> _shadowMap;
    std::array<std::shared_ptr<typename API::RTT>, 2> _dofBuffer;
    std::unique_ptr<typename API::RTT> _godRayBuffer;
    StackPOD<Mat4f> _vpLightVolume;
    typename API::RendertargetStack _renderTargets;
    unsigned _defaultRenderTarget = 0;
    Mat4f _vpScene;
    bool _offScreenRendering;
    bool _shadowMapping;
    float _acc = 0.f;
    float _dt = 1.f / 30.f;
#if RENDERER_STATS
    RendererStats _stats;
#endif
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::vector<std::shared_ptr<StaticMeshRenderable<API, BV>>> _staticMeshRenderables;
    std::vector<std::shared_ptr<StaticInstancedMeshRenderable<API, BV>>> _staticInstancedMeshRenderables;
    std::shared_ptr<SkydomeRenderable<API, BV>> _skydomeRenderable;
    StackPOD<IMeshRenderable<API, BV>*> _fullyVisibleMeshes;
    StackPOD<IMeshRenderable<API, BV>*> _fullyVisibleMeshesAsync;
    StackPOD<IMeshRenderable<API, BV>*> _intersectedMeshes;
    StackPOD<IMeshRenderable<API, BV>*> _intersectedMeshesAsync;
    StackPOD<IMeshRenderable<API, BV>*> _visibleMeshes;
    StackPOD<IMeshRenderable<API, BV>*> _visibleMeshesAsync;
    //StackPOD<ShaderDesc<API, BV>*> _displayList;
    std::map<ShaderDesc<API>*, std::map<MaterialDesc<API>*, StackPOD<IMeshRenderable<API, BV>*>>> _displayList;
    BV _sceneBounds;
    std::unique_ptr<BVH> _bvhStatic;
    SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc<API>>, const std::shared_ptr<Material>&, const GraphicsSettings&> _matDescCache;
    SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&> _shaderDescCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&> _shaderCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&> _textureCache;
    void renderBVHNodes(Camera render_cam, Camera cull_cam)
    {
      cull_cam.extractFrustumPlanes(_gsp._projectionMatrix * cull_cam.getViewMatrix(), _api.getZNearMapping());
      StackPOD<typename BVH::Node*> visible_nodes;
      _bvhStatic->cullVisibleNodes(cull_cam.getCullingParams(), visible_nodes);
      if (visible_nodes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        StackPOD<BV const *> bvs;
        bvs.reserve(visible_nodes.size());
        for (const auto& n : visible_nodes) {
          bvs.push_back(&n->getBV());
        }
        _api.renderBVs(bvs, _gsp._projectionMatrix * render_cam.getViewMatrix(), Vec3f(1.f, 0.f, 0.f));
      }
    }
    void renderObjectBVs()
    {
      if (_visibleMeshes.size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        StackPOD<BV const *> bvs;
        bvs.reserve(_visibleMeshes.size());
        for (auto m : _visibleMeshes) {
          bvs.push_back(&m->getBV());
        }
        _api.renderBVs(bvs, _vpScene, Vec3f(0.f, 1.f, 0.f));
      }
    }
    void renderScene(const StackPOD<IMeshRenderable<API, BV>*>& visible_meshes)
    {
#if RENDERER_STATS
      Timing timing;
#endif
      groupMeshes(visible_meshes);
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
      _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], _pp._near, _pp._fieldOfViewDegrees,
        _debugCamera ? inverse(_debugCamera->updateViewMatrix(_debugCamera->getPosition(), _debugCamera->getEulerAngles())) : inverse(_gsp._viewMatrix),
        static_cast<float>(_gs->getShadowMapSize()), _gs->getFrustumSplits(), _api.getZNearMapping(), _gsp._worldToLight, _vpLightVolume);
      _api.setDepthClampEnabled<true>();
      _api.enablePolygonOffset(_gs->getShadowPolygonOffsetFactor(), _gs->getShadowPolygonOffsetUnits());
      _api.setViewport(Vec2u(_gs->getShadowMapSize()));
      _renderTargets.clear();
      for (unsigned i = 0; i < _gs->getFrustumSplits().size(); i++) {
#if RENDERER_STATS
        Timing timing;
#endif
        cullMeshes(_vpLightVolume[i], _debugCamera ? *_debugCamera : *_camera, _fullyVisibleMeshes, _intersectedMeshes, _visibleMeshes);
#if RENDERER_STATS
        _stats._cullingShadowMapMicroSeconds += timing.duration<std::chrono::microseconds>();
#endif
#if RENDERER_STATS
        timing.start();
#endif
        groupMeshes<true>(_visibleMeshes);
#if RENDERER_STATS
        _stats._shadowMapGroupingMicroSeconds += timing.duration<std::chrono::microseconds>();
        timing.start();
#endif
        _api.setRendertargets(_renderTargets, _shadowMap.get(), i);
        _api.clearRendertarget(false, true, false);
        _gsp._VP = &_gsp._worldToLight[i];
        auto stats = renderMeshes<true>();
#if RENDERER_STATS
        _stats._shadowMapRenderCPUMicroSeconds += timing.duration<std::chrono::microseconds>();
        _stats._renderedMeshesShadow += stats._renderedMeshes;
        _stats._renderedTrianglesShadow += stats._renderedTriangles;
#endif
      }
      _api.disablePolygonOffset();
      _gsp._smFrustumSplits = &_gs->getFrustumSplits();
      _gsp._shadowDarkenFactor = _gs->getShadowDarkenFactor();
      _api.setDepthClampEnabled<false>();
      _gsp._viewMatrixThirdRow = _debugCamera ? _debugCamera->getViewMatrix().row(2) : _gsp._viewMatrix.row(2);
    }
    void graphicsSettingsChanged()
    {
      _shaderCache.clear();
      _shaderDescCache.clear();
      _api.createCompositeShader(*_gs);
    }
    inline unsigned elementsPerThread(unsigned num_elements, unsigned num_threads) const
    {
      return static_cast<unsigned>(std::ceil(static_cast<float>(num_elements) / static_cast<float>(num_threads)));
    }
    inline void cullMeshes(const Mat4f& view_projection_matrix, Camera camera,
      StackPOD<IMeshRenderable<API, BV>*>& fully_visible_meshes, StackPOD<IMeshRenderable<API, BV>*>& intersected_meshes, StackPOD<IMeshRenderable<API, BV>*>& visible_meshes)
    {
      camera.extractFrustumPlanes(view_projection_matrix, _api.getZNearMapping());
      fully_visible_meshes.clear();
      intersected_meshes.clear();
      visible_meshes.clear();
      auto cp = camera.getCullingParams();
      _bvhStatic->cullVisibleObjects(cp, fully_visible_meshes, intersected_meshes);
      if (_staticInstancedMeshRenderables.size()) {
        _api.prepareCulling(cp._frustumPlanes, cp._camPos);
      }
      auto num_threads = std::thread::hardware_concurrency();
      unsigned num_meshes = static_cast<unsigned>(fully_visible_meshes.size());
      auto elements_per_thread = elementsPerThread(num_meshes, num_threads);
      if (_gs->getMultithreadedDetailCulling() && elements_per_thread >= 256) {
        std::vector<std::future<StackPOD<IMeshRenderable<API, BV>*>>> futures;
        futures.reserve(num_threads);
        unsigned j = 0;
        const auto& meshes_to_cull = fully_visible_meshes;
        for (unsigned i = 0; i < num_threads; i++) {
          unsigned max_index = std::min(j + elements_per_thread, num_meshes);
          futures.push_back(std::async(std::launch::async, [j, max_index, cp, &meshes_to_cull]() {
            StackPOD<IMeshRenderable<API, BV>*> meshes;
            meshes.reserve(max_index - j);
            for (unsigned i = j; i < max_index; i++) {
              if (meshes_to_cull[i]->cull(cp)) {
                meshes.push_back(meshes_to_cull[i]);
              }
            }
            return meshes;
          }));
          j += elements_per_thread;
        }
        for (auto& f : futures) {
          visible_meshes.append(f.get());
        }
      }
      else {
        for (const auto& m : fully_visible_meshes) {
          if (m->cull(cp)) {
            visible_meshes.push_back(m);
          }
        }
      }
      for (const auto& m : intersected_meshes) { // No need to multithread intersected meshes, because the amount is usually much smaller compared to fully visible meshes.
        if (m->cullAndIntersect(cp)) {
          visible_meshes.push_back(m);
        }
      }
      if (_staticInstancedMeshRenderables.size()) {
        _api.endCulling();
      }
    }
    template<bool depth = false>
    inline void groupMeshes(const StackPOD<IMeshRenderable<API, BV>*>& visible_meshes)
    {
      for (const auto& m : visible_meshes) {
        _displayList[depth ? m->getShaderDescDepth()->get() : m->getShaderDesc()->get()][m->getMaterialDesc()].push_back_secure(m);
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
      for (const auto& shader : _displayList) {
        shader.first->setup(_gsp);
        for (const auto& material : shader.second) {
          material.first->setup<depth>();
          for (const auto& mr : material.second) {
            depth ? mr->renderDepth(_api) : mr->render(_api);
#if RENDERER_STATS
            stats._renderedTriangles += mr->numTriangles();
            stats._renderedMeshes += mr->numMeshes();
#endif
          }
        }
      }
      _displayList.clear();
      return stats;
    }
  };
}

#endif
