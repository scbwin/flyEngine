#ifndef RENDERER_H
#define RENDERER_H

#include <math/MathHelpers.h>
#include <System.h>
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
#include <RenderList.h>
#include <PtrCache.h>

#define RENDERER_STATS 1

namespace fly
{
  template<typename API, typename BV>
  class Renderer : public System, public GraphicsSettings::Listener
  {
  public:
    using MeshRenderable = IMeshRenderable<API, BV>;
    using RenderList = RenderList<API, BV>;
    using MeshRenderablePtr = MeshRenderable * ;
    using MaterialDescCache = PtrCache<std::shared_ptr<Material>, MaterialDesc<API>, const std::shared_ptr<Material>&, const GraphicsSettings&>;
    using ShaderSource = typename API::ShaderSource;
#if RENDERER_STATS
    struct CullingStats
    {
      unsigned _bvhTraversalMicroSeconds;
      unsigned _fineCullingMicroSeconds;
    };
    struct RendererStats
    {
      unsigned _renderedTriangles;
      unsigned _renderedTrianglesShadow;
      unsigned _renderedMeshes;
      unsigned _renderedMeshesShadow;
      CullingStats _cullStats;
      CullingStats _cullStatsSM;
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
      _materialDescCache([this](const std::shared_ptr<Material>& material, const GraphicsSettings& gs) {
      return new MaterialDesc<API>(material, _api, gs, &_textureCache, &_shaderDescCache, &_shaderCache);
    }),
      _shaderDescCache([](const std::shared_ptr<typename API::Shader>& shader, unsigned flags, API& api) {
      return new ShaderDesc<API>(shader, flags, api);
    }),
      _textureCache([](const std::string& path) {
      return API::createTexture(path);
    }),
      _shaderCache([](ShaderSource& vertex_source, ShaderSource& fragment_source, ShaderSource& geometry_source) {
      return API::createShader(vertex_source, fragment_source, geometry_source);
    })
    {
      _renderTargets.reserve(_api._maxRendertargets);
    }
    virtual ~Renderer() 
    {
      std::cout << "~Renderer()" << std::endl;
      if (_shaderCache.size() || _shaderDescCache.size() || _materialDescCache.size() || _textureCache.size()) {
        std::cout << "Error: Potential space leak detected!" << std::endl;
        std::cout << "Num shader descriptions:" << _shaderDescCache.size() << std::endl;
        std::cout << "Num material descriptions:" << _materialDescCache.size() << std::endl;
        std::cout << "Num shaders:" << _shaderCache.size() << std::endl;
        std::cout << "Num textures:" << _textureCache.size() << std::endl;
        abort();
      }
    }
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
    void addStaticMeshRenderable(MeshRenderablePtr smr) 
    {
      _meshRenderables.push_back(smr);
      _sceneBounds = _sceneBounds.getUnion(smr->getBV());
    }
    void addStaticMeshRenderables(const std::vector<MeshRenderablePtr>& smrs)
    {
      _meshRenderables.insert(_meshRenderables.end(), smrs.begin(), smrs.end());
      for (const auto& smr : smrs) {
        _sceneBounds = _sceneBounds.getUnion(smr->getBV());
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
      _multiThreadedCulling = _gs->getMultithreadedCulling() && _shadowMapping;
      _renderListScene = _multiThreadedCulling ? &_renderListAsync : &_renderList;
      _api.beginFrame();
      _gsp._camPosworld = _camera->getPosition();
      _gsp._viewMatrix = _camera->updateViewMatrix();
      _gsp._projectionMatrix = MathHelpers::getProjectionMatrixPerspective(_camera->getParams()._fovDegrees, _viewPortSize[0] / _viewPortSize[1], _camera->getParams()._near, _camera->getParams()._far, _api.getZNearMapping());
      (*_cullCamera)->updateViewMatrix();
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
      auto cull_vp = _gsp._projectionMatrix * (*_cullCamera)->getViewMatrix();
      if (_multiThreadedCulling) {
        async = std::async(std::launch::async, [this, cull_vp]() {
          _stats._cullStats = cullMeshes(cull_vp, **_cullCamera, *_renderListScene, _cullResultAsync);
        });
      }
      if (_shadowMapping) {
        renderShadowMap();
      }
      if (_multiThreadedCulling) {
        {
#if RENDERER_STATS
          Timing timing;
#endif
          async.get();
#if RENDERER_STATS
          _stats._rendererIdleTimeMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
        }
        cullGPU(*_renderListScene, **_cullCamera, cull_vp);
        selectLod(*_renderListScene, **_cullCamera);
      }
      else {
        _stats._cullStats = cullMeshes(cull_vp, **_cullCamera, *_renderListScene, _cullResult);
        cullGPU(*_renderListScene, **_cullCamera, cull_vp);
        selectLod(*_renderListScene, **_cullCamera);
      }
      _api.setViewport(_viewPortSize);
      _gsp._VP = &_vpScene;
      if (_gs->depthPrepassEnabled()) {
        _renderTargets.clear();
        _api.setRendertargets(_renderTargets, _depthBuffer.get());
        _api.clearRendertarget(false, true, false);
        groupMeshes<true>(_renderListScene->getVisibleMeshes());
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
        _api.renderSkydome(skydome_vp, _skydomeRenderable->getMeshData());
        _api.setCullMode<API::CullMode::BACK>();
      }
      if (_gs->getDebugBVH()) {
        renderBVHNodes(*_camera, **_cullCamera);
      }
      if (_gs->getDebugObjectBVs()) {
        renderObjectBVs();
      }
      if (_offScreenRendering || _debugCamera) {
        _api.setDepthTestEnabled<false>();
      }
      if (_debugCamera) {
        _api.renderDebugFrustum(_gsp._projectionMatrix * _debugCamera->getViewMatrix(), _vpScene);
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
        auto light_pos_world = _gsp._camPosworld + *_gsp._lightDirWorld * -_camera->getParams()._far;
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
    using BVH = KdTree<MeshRenderable*, BV>;
    const std::unique_ptr<BVH>& getStaticBVH() const
    {
      return _bvhStatic;
    }
    const BV& getSceneBounds() const
    {
      return _sceneBounds;
    }
    std::shared_ptr<MaterialDesc<API>> createMaterialDesc(const std::shared_ptr<Material>& material)
    {
      std::shared_ptr<MaterialDesc<API>> ret;
      if (!_materialDescCache.getOrCreate(material, ret, material, *_gs)) {
        _gs->addListener(ret);
      }
      return ret;
    }
    typename API::MeshData addMesh(const std::shared_ptr<Mesh>& mesh)
    {
      return _meshGeometryStorage.addMesh(mesh);
    }
    void setDebugCamera(const std::shared_ptr<Camera>& camera)
    {
      _debugCamera = camera;
      _cullCamera = _debugCamera ? &_debugCamera : &_camera;
    }
    const std::shared_ptr<Camera>& getDebugCamera() const
    {
      return _debugCamera;
    }
    void buildBVH()
    {
      _cullResult.reserve(_meshRenderables.size());
      _cullResultAsync.reserve(_cullResult.capacity());
      std::cout << "Mesh renderables:" << _meshRenderables.size() << std::endl;
      if (!_cullResult.capacity()) {
        throw std::exception("No meshes were added to the renderer.");
      }
      Timing timing;
      _bvhStatic = std::make_unique<BVH>(_meshRenderables);
      std::cout << "BVH construction took " << timing.duration<std::chrono::milliseconds>() << " milliseconds." << std::endl;
      std::cout << "BVH takes " << static_cast<float>(_bvhStatic->getSizeInBytes()) / 1024.f / 1024.f << " MB memory" << std::endl;
      std::cout << "Scene bounds:" << _bvhStatic->getBV() << std::endl;
      unsigned internal_nodes, leaf_nodes;
      _bvhStatic->countNodes(internal_nodes, leaf_nodes);
      std::cout << "BVH internal nodes:" << internal_nodes << std::endl;
      std::cout << "BVH leaf nodes:" << leaf_nodes << std::endl;
    }
  private:
    API _api;
    GlobalShaderParams _gsp;
    Vec2f _viewPortSize = Vec2f(1.f);
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    std::shared_ptr<Camera> _debugCamera;
    std::shared_ptr<Camera>* _cullCamera = &_camera;
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
    bool _multiThreadedCulling;
    BV _sceneBounds;
#if RENDERER_STATS
    RendererStats _stats;
#endif
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::vector<MeshRenderablePtr> _meshRenderables;
    std::shared_ptr<SkydomeRenderable<API, BV>> _skydomeRenderable;
    CullResult<MeshRenderable*> _cullResult;
    CullResult<MeshRenderable*> _cullResultAsync;
    RenderList _renderList;
    RenderList _renderListAsync;
    RenderList* _renderListScene;
    std::map<ShaderDesc<API> const *, std::map<MaterialDesc<API> const *, StackPOD<MeshRenderable const*>>> _displayList;
    std::unique_ptr<BVH> _bvhStatic;
    typename MaterialDesc<API>::TextureCache _textureCache;
    typename MaterialDesc<API>::ShaderCache _shaderCache;
    typename MaterialDesc<API>::ShaderDescCache _shaderDescCache;
    MaterialDescCache _materialDescCache;
    void renderBVHNodes(Camera render_cam, Camera cull_cam)
    {
      cull_cam.extractFrustumPlanes(_gsp._projectionMatrix * cull_cam.getViewMatrix(), _api.getZNearMapping());
      StackPOD<typename BVH::Node const *> visible_nodes;
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
      if (_renderListScene->getVisibleMeshes().size()) {
        _api.setDepthWriteEnabled<true>();
        _api.setDepthFunc<API::DepthFunc::LEQUAL>();
        StackPOD<BV const *> bvs;
        bvs.reserve(_renderListScene->getVisibleMeshes().size());
        for (auto m : _renderListScene->getVisibleMeshes()) {
          bvs.push_back(&m->getBV());
        }
        _api.renderBVs(bvs, _vpScene, Vec3f(0.f, 1.f, 0.f));
      }
    }
    void renderScene()
    {
#if RENDERER_STATS
      Timing timing;
#endif
      groupMeshes(_renderListScene->getVisibleMeshes());
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
      _directionalLight->getViewProjectionMatrices(_viewPortSize[0] / _viewPortSize[1], (*_cullCamera)->getParams()._near, (*_cullCamera)->getParams()._fovDegrees, inverse((*_cullCamera)->getViewMatrix()),
        static_cast<float>(_gs->getShadowMapSize()), _gs->getFrustumSplits(), _api.getZNearMapping(), _gsp._worldToLight, _vpLightVolume);
      _api.setDepthClampEnabled<true>();
      _api.enablePolygonOffset(_gs->getShadowPolygonOffsetFactor(), _gs->getShadowPolygonOffsetUnits());
      _api.setViewport(Vec2u(_gs->getShadowMapSize()));
      _renderTargets.clear();
      for (unsigned i = 0; i < _gs->getFrustumSplits().size(); i++) {
        {
          auto stats = cullMeshes(_vpLightVolume[i], **_cullCamera, _renderList, _cullResult);
#if RENDERER_STATS
          _stats._cullStatsSM._bvhTraversalMicroSeconds += stats._bvhTraversalMicroSeconds;
          _stats._cullStatsSM._fineCullingMicroSeconds += stats._fineCullingMicroSeconds;
#endif
          cullGPU(_renderList, **_cullCamera, _vpLightVolume[i]);
          selectLod(_renderList, **_cullCamera);
        }
#if RENDERER_STATS
        Timing timing;
#endif
        groupMeshes<true>(_renderList.getVisibleMeshes());
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
    inline CullingStats cullMeshes(const Mat4f& view_projection_matrix, Camera camera,
      RenderList& renderlist, CullResult<MeshRenderable*>& cull_result)
    {
      camera.extractFrustumPlanes(view_projection_matrix, _api.getZNearMapping());
      renderlist.clear();
      cull_result.clear();
      auto cp = camera.getCullingParams();
      CullingStats stats;
      {
#if RENDERER_STATS
        Timing timing;
#endif
        _bvhStatic->cullVisibleObjects(cp, cull_result);
#if RENDERER_STATS
        stats._bvhTraversalMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      }
#if RENDERER_STATS
      Timing timing;
#endif
      renderlist.reserve(cull_result.size());
      auto num_threads = std::thread::hardware_concurrency();
      unsigned num_meshes = static_cast<unsigned>(cull_result._fullyVisibleObjects.size());
      auto elements_per_thread = elementsPerThread(num_meshes, num_threads);
      if (_gs->getMultithreadedDetailCulling() && elements_per_thread >= 256) {
        std::vector<std::future<RenderList>> futures;
        futures.reserve(num_threads);
        unsigned start = 0;
        const auto& meshes_to_cull = cull_result._fullyVisibleObjects;
        for (unsigned i = 0; i < num_threads; i++) {
          unsigned end = std::min(start + elements_per_thread, num_meshes);
          futures.push_back(std::async(std::launch::async, [start, end, cp, &meshes_to_cull]() {
            RenderList renderlist;
            renderlist.reserve(end - start);
            for (unsigned i = start; i < end; i++) {
              meshes_to_cull[i]->addIfLargeEnough(cp, renderlist);
            }
            return renderlist;
          }));
          start += elements_per_thread;
        }
        for (auto& f : futures) {
          renderlist.append(f.get());
        }
      }
      else {
        for (const auto& m : cull_result._fullyVisibleObjects) {
          m->addIfLargeEnough(cp, renderlist);
        }
      }
      for (const auto& m : cull_result._probablyVisibleObjects) { // No need to multithread probably visible meshes, because the amount is usually much smaller compared to fully visible meshes.
        m->addIfLargeEnoughAndVisible(cp, renderlist);
      }
#if RENDERER_STATS
      stats._fineCullingMicroSeconds = timing.duration<std::chrono::microseconds>();
#endif
      return stats;
    }
    inline void cullGPU(const RenderList& renderlist, Camera camera, const Mat4f& view_projection_matrix)
    {
      if (renderlist.getGPUCullList().size() || renderlist.getGPULodList().size()) {
        camera.extractFrustumPlanes(view_projection_matrix, _api.getZNearMapping());
        auto cp = camera.getCullingParams();
        if (renderlist.getGPUCullList().size()) {
          _api.prepareCulling(cp._frustumPlanes, cp._camPos, cp._lodRange, cp._thresh);
          for (const auto& m : renderlist.getGPUCullList()) {
            m->cullGPU(_api);
          }
        }
        if (renderlist.getGPULodList().size()) {
          _api.prepareLod(cp._camPos, cp._lodRange, cp._thresh);
          for (const auto& m : renderlist.getGPULodList()) {
            m->cullGPU(_api);
          }
        }
        _api.endCulling();
      }
    }
    inline void selectLod(const RenderList& renderlist, const Camera& camera)
    {
      if (!renderlist.getCPULodList().size()) {
        return;
      }
      auto cp = camera.getCullingParams();
      for (const auto& m : renderlist.getCPULodList()) {
        m->selectLod(cp);
      }
    }
    template<bool depth = false>
    inline void groupMeshes(const StackPOD<MeshRenderable const *>& visible_meshes)
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
