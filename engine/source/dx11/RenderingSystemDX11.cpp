#include <opencv2/opencv.hpp>
#include <dx11/RenderingSystemDX11.h>
#include <dx11/DXUtils.h>
#include <d3dcompiler.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Material.h>
#include <Renderables.h>
#include <StaticModelRenderable.h>
#include <Camera.h>
#include <Entity.h>
#include <Light.h>
#include <Billboard.h>
#include <TerrainNew.h>
#include <Mesh.h>
#include <Model.h>
#include <Transform.h>
#include <physics/ParticleSystem.h>
#include <dx11/DX11States.h>
#include <CommonStates.h>
#include <Quadtree.h>
#include <Timing.h>

namespace fly
{
  RenderingSystemDX11::RenderingSystemDX11(HWND window, const std::array<Vec2f, 2>& quadtree_min_max)
  {
    D3D_FEATURE_LEVEL feature_level;
    UINT device_flags = 0;
#if defined(DEBUG) | defined(_DEBUG)
    device_flags |= D3D11_CREATE_DEVICE_FLAG::D3D11_CREATE_DEVICE_DEBUG;
#endif
    HR(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, nullptr, 0, D3D11_SDK_VERSION, &_device, &feature_level, &_context));
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD;
    desc.Windowed = true;

    CComPtr<IDXGIDevice> dxgi_device;
    HR(_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device)));

    _dxgiAdapter = nullptr;
    HR(dxgi_device->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&_dxgiAdapter)));

    CComPtr<IDXGIFactory> dxgi_factory;
    HR(_dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgi_factory)));

    HR(dxgi_factory->CreateSwapChain(_device, &desc, &_swapChain));

    initEffects();

    setSettings({ true, true, false, false, false, false, false, glm::vec3(0.f, 4.f, 5250.f), glm::vec3(13.f, 24.f, 42.f) / 255.f, 1.9f, 3.2f, 1.f, 8, 16, 3.f, 24.f, 10000, 1.f, {1.f, 1.f} });

    _commonStates = std::make_unique<DirectX::CommonStates>(_device);
    _dx11States = std::make_unique<DX11States>(_device);
  }

  RenderingSystemDX11::~RenderingSystemDX11()
  {
  }

  void RenderingSystemDX11::onComponentsChanged(Entity* entity)
  {
    auto model = entity->getComponent<Model>();
    auto transform = entity->getComponent<Transform>();
    auto cam = entity->getComponent<Camera>();
    auto dl = entity->getComponent<DirectionalLight>();
    auto particle_system = entity->getComponent<ParticleSystem>();
    auto billboard = entity->getComponent<Billboard>();
    auto smr = entity->getComponent<StaticModelRenderable>();
    auto terrain = entity->getComponent<TerrainNew>();
    auto ptr = entity->getComponent<ProceduralTerrainRenderable>();
    auto sbr = entity->getComponent<SkyboxRenderable>();

    if (billboard && transform && particle_system) {
      _particleBillboardRenderables[entity] = { billboard, particle_system };
      createTexture(L"assets/flames.png", _particleBillboardRenderables[entity]._srv, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_SRGB);
    }
    else {
      _particleBillboardRenderables.erase(entity);
    }

    if (model && transform && particle_system) {
      if (!_modelDataCache.count(model)) {
        _modelDataCache[model] = std::make_shared<ModelData>(model, this);
      }
      _particleModelRenderables[entity] = { std::make_shared<DX11StaticModelRenderable>(smr, this), particle_system };
    }
    if (smr) {
      _staticModelRenderables[entity] = std::make_shared<DX11StaticModelRenderable>(smr, this);
      _sceneMin = minimum(_sceneMin, _staticModelRenderables[entity]->getAABBWorld()->getMin());
      _sceneMax = maximum(_sceneMax, _staticModelRenderables[entity]->getAABBWorld()->getMax());
    }
    if (!particle_system) {
      _particleModelRenderables.erase(entity);
    }
    if (!smr) {
      _staticModelRenderables.erase(entity);
    }

    if (cam) {
      _camera = cam;
      _camPos = _camera->_pos;
      _camEulerAngles = glm::quat(_camera->_eulerAngles);
    }

    if (dl) {
      _directionalLight._dl = dl;
      _directionalLight._shadowMap = std::make_unique<ShadowMap>(this);
      HR(_fxShadowMap->SetResource(_directionalLight._shadowMap->_srv));
    }
    if (terrain && ptr) {
      _proceduralTerrainRenderable = std::make_unique<DX11ProceduralTerrainRenderable>(terrain, ptr, this);
    }
    if (sbr && model) {
      _skyboxRenderable = std::make_unique<DX11SkyboxRenderable>(model, this);
    }
  }

  void RenderingSystemDX11::update(float time, float delta_time)
  {
    prepareRender(time, delta_time);
    renderShadowMaps();
    renderScene();
    postProcessing();
    composite();
  }

  void RenderingSystemDX11::onResize(const Vec2u& size)
  {
    _viewportSize = size;
    _aspectRatio = static_cast<float>(size[0]) / size[1];
    _projectionMatrix = glm::perspectiveZO(glm::radians(_fov), _aspectRatio, _near, _far);
    _PInverse = glm::inverse(glm::mat4(_projectionMatrix));
    HR(_fxP->SetMatrixTranspose(_projectionMatrix.ptr()));
    HR(_fxPInverse->SetMatrixTranspose(_PInverse.ptr()));
    //   HR(_fxPInverseTerrain->SetMatrixTranspose(&_PInverse[0][0]));

    _backBufferRtv = nullptr;
    _depthStencilView = nullptr;
    _depthStencilSrv = nullptr;
    _lightingBuffer = std::make_unique<RTT>(size, this);

    // Resize back_buffer
    HR(_swapChain->ResizeBuffers(2, size[0], size[1], DXGI_FORMAT_UNKNOWN, 0));

    // Recreate render target view
    CComPtr<ID3D11Texture2D> back_buffer;
    HR(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)));
    HR(_device->CreateRenderTargetView(back_buffer, 0, &_backBufferRtv));

    // Create depth/stencil view
    _dsTextureDesc.Width = size[0];
    _dsTextureDesc.Height = size[1];
    _dsTextureDesc.MipLevels = 1;
    _dsTextureDesc.ArraySize = 1;
    _dsTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    _dsTextureDesc.SampleDesc.Count = 1;
    _dsTextureDesc.SampleDesc.Quality = 0;
    _dsTextureDesc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
    _dsTextureDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
    _dsTextureDesc.CPUAccessFlags = 0;
    _dsTextureDesc.MiscFlags = 0;
    CComPtr<ID3D11Texture2D> depth_stencil_tex;
    HR(_device->CreateTexture2D(&_dsTextureDesc, nullptr, &depth_stencil_tex));
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
    HR(_device->CreateDepthStencilView(depth_stencil_tex, &dsv_desc, &_depthStencilView));
    _dsSrvDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    _dsSrvDesc.Texture2D.MipLevels = 1;
    _dsSrvDesc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
    HR(_device->CreateShaderResourceView(depth_stencil_tex, &_dsSrvDesc, &_depthStencilSrv));

    _viewPort = {};
    _viewPort.Width = static_cast<float>(size[0]);
    _viewPort.Height = static_cast<float>(size[1]);
    _viewPort.MaxDepth = 1.f;
    _viewPort.MinDepth = 0.f;
    _viewPort.TopLeftX = 0.f;
    _viewPort.TopLeftY = 0.f;

    initAdditionalRenderTargets();
  }

  void RenderingSystemDX11::setSSRBlendWeight(float alpha)
  {
    _ssrBlendWeight = alpha;
  }

  float RenderingSystemDX11::getSSRBlendWeight() const
  {
    return _ssrBlendWeight;
  }

  void RenderingSystemDX11::initEffects()
  {
    _effects = std::make_unique<Effects>(L"assets/dx11/effect.fx", _device, L"assets/dx11/effects_terrain.fx");
    std::vector<D3D11_INPUT_ELEMENT_DESC> vertex_desc = {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(fly::Vertex, _position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(fly::Vertex, _normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(fly::Vertex, _uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(fly::Vertex, _tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(fly::Vertex, _bitangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    _brightPass = _effects->getEffect()->GetTechniqueByName("brightPassTech")->GetPassByIndex(0);
    _gaussPassHor = _effects->getEffect()->GetTechniqueByName("gaussTech")->GetPassByName("passHorizontal");
    _gaussPassVert = _effects->getEffect()->GetTechniqueByName("gaussTech")->GetPassByName("passVertical");
    _copyPass = _effects->getEffect()->GetTechniqueByName("copyTech")->GetPassByIndex(0);
    _lightSourcePass = _effects->getEffect()->GetTechniqueByName("lightSourceTech")->GetPassByIndex(0);
    _ssrPass = _effects->getEffect()->GetTechniqueByName("ssrTech")->GetPassByIndex(0);
    _lightParticlePass = _effects->getEffect()->GetTechniqueByName("lightParticleTech")->GetPassByIndex(0);
    _billboardPass = _effects->getEffect()->GetTechniqueByName("billboardTech")->GetPassByIndex(0);
    _minMaxPass = _effects->getEffect()->GetTechniqueByName("minMaxTech")->GetPassByIndex(0);
    _minMaxPass2 = _effects->getEffect()->GetTechniqueByName("minMaxTech2")->GetPassByIndex(0);
    auto pass_desc = _effects->getMeshPassDesc();
    HR(_device->CreateInputLayout(&vertex_desc.front(), static_cast<unsigned>(vertex_desc.size()), pass_desc.pIAInputSignature, pass_desc.IAInputSignatureSize, &_defaultInputLayout));
    _fxMVInverseTranspose = _effects->getEffect()->GetVariableByName("MVInvTranspose")->AsMatrix();
    _fxM = _effects->getEffect()->GetVariableByName("M")->AsMatrix();
    _fxV = _effects->getEffect()->GetVariableByName("V")->AsMatrix();
    _fxVInverse = _effects->getEffect()->GetVariableByName("VInverse")->AsMatrix();
    _fxP = _effects->getEffect()->GetVariableByName("P")->AsMatrix();
    _fxPInverse = _effects->getEffect()->GetVariableByName("PInverse")->AsMatrix();
    _fxVPInverse = _effects->getEffect()->GetVariableByName("VPInverse")->AsMatrix();
    _fxVPBefore = _effects->getEffect()->GetVariableByName("VPBefore")->AsMatrix();
    _fxVPInverseVPBefore = _effects->getEffect()->GetVariableByName("VPInverseVPBefore")->AsMatrix();
    _fxVP = _effects->getEffect()->GetVariableByName("VP")->AsMatrix();
    _fxLightMVP = _effects->getEffect()->GetVariableByName("lightMVP")->AsMatrix();
    _fxLightVPs = _effects->getEffect()->GetVariableByName("lightVPs")->AsMatrix();
    _fxMVPs = _effects->getEffect()->GetVariableByName("MVPs")->AsMatrix();
    _fxMVP = _effects->getEffect()->GetVariableByName("MVP")->AsMatrix();
    _fxCascadeIndex = _effects->getEffect()->GetVariableByName("cascadeIndex")->AsScalar();
    _fxCascDistances = _effects->getEffect()->GetVariableByName("cascadeDistances")->AsScalar();
    _fxDiffuseTex = _effects->getEffect()->GetVariableByName("diffuseTexture")->AsShaderResource();
    _fxAlphaMap = _effects->getEffect()->GetVariableByName("alphaTexture")->AsShaderResource();
    _fxNormalMap = _effects->getEffect()->GetVariableByName("normalMap")->AsShaderResource();
    _fxLightingTex = _effects->getEffect()->GetVariableByName("lightingTexture")->AsShaderResource();
    _fxDepthTex = _effects->getEffect()->GetVariableByName("depthTexture")->AsShaderResource();
    _fxShadowMap = _effects->getEffect()->GetVariableByName("shadowMap")->AsShaderResource();
    _fxBlurInputTex = _effects->getEffect()->GetVariableByName("textureToBlur")->AsShaderResource();
    _fxTexToCopy = _effects->getEffect()->GetVariableByName("textureToCopy")->AsShaderResource();
    _fxLensflareTexture = _effects->getEffect()->GetVariableByName("lensflareTexture")->AsShaderResource();
    _fxDofTexture = _effects->getEffect()->GetVariableByName("dofTexture")->AsShaderResource();
    _fxVsNormalsTexture = _effects->getEffect()->GetVariableByName("vsNormalsTexture")->AsShaderResource();
    _fxVsZTexture = _effects->getEffect()->GetVariableByName("vsZTexture")->AsShaderResource();
    _fxMinMaxTexture = _effects->getEffect()->GetVariableByName("minMaxTexture")->AsShaderResource();
    _fxMinMaxTexture2 = _effects->getEffect()->GetVariableByName("minMaxTexture2")->AsShaderResource();
    _fxDiffuseColor = _effects->getEffect()->GetVariableByName("diffuseColor")->AsVector();
    _fxLightPosView = _effects->getEffect()->GetVariableByName("lightPosView")->AsVector();
    _fxLightColor = _effects->getEffect()->GetVariableByName("lightColor")->AsVector();
    _fxTime = _effects->getEffect()->GetVariableByName("time")->AsScalar();
    _fxMotionBlurStrength = _effects->getEffect()->GetVariableByName("motionBlurStrength")->AsScalar();
    _fxWindPivotMinY = _effects->getEffect()->GetVariableByName("windPivotMinY")->AsScalar();
    _fxWindPivotMaxY = _effects->getEffect()->GetVariableByName("windPivotMaxY")->AsScalar();
    _fxTexelSize = _effects->getEffect()->GetVariableByName("texelSize")->AsVector();
    _fxQuadPos = _effects->getEffect()->GetVariableByName("quadPos")->AsVector();
    _fxQuadScale = _effects->getEffect()->GetVariableByName("quadScale")->AsVector();
    _fxWindStrength = _effects->getEffect()->GetVariableByName("windStrength")->AsScalar();
    _fxBillboardPosWorld = _effects->getEffect()->GetVariableByName("billboardPosWorld")->AsVector();
    _fxCamRightWorld = _effects->getEffect()->GetVariableByName("camRightWorld")->AsVector();
    _fxCamUpWorld = _effects->getEffect()->GetVariableByName("camUpWorld")->AsVector();
    _fxBillboardSize = _effects->getEffect()->GetVariableByName("billboardSize")->AsVector();
    _fxFades = _effects->getEffect()->GetVariableByName("fades")->AsScalar();
    _fxCamPosWorld = _effects->getEffect()->GetVariableByName("camPosWorld")->AsVector();
    _fxminMaxArraySlice = _effects->getEffect()->GetVariableByName("minMaxArraySlice")->AsScalar();
    _fxWindFrequency = _effects->getEffect()->GetVariableByName("windFrequency")->AsScalar();
    _fxDepthOfFieldDistances = _effects->getEffect()->GetVariableByName("depthOfFieldDistances")->AsVector();
    _fxSkycolor = _effects->getEffect()->GetVariableByName("skyColor")->AsVector();
    _fxBrightScale = _effects->getEffect()->GetVariableByName("brightScale")->AsScalar();
    _fxBrightBias = _effects->getEffect()->GetVariableByName("brightBias")->AsScalar();
    _fxExposure = _effects->getEffect()->GetVariableByName("exposure")->AsScalar();
    _fxNumCascades = _effects->getEffect()->GetVariableByName("numCascades")->AsScalar();
    _fxssrSteps = _effects->getEffect()->GetVariableByName("ssrSteps")->AsScalar();
    _fxssrMinRayLen = _effects->getEffect()->GetVariableByName("ssrMinRayLen")->AsScalar();
    _fxssrRayLenScale = _effects->getEffect()->GetVariableByName("ssrRayLenScale")->AsScalar();

    _fxMVPTerrain = _effects->getTerrainEffect()->GetVariableByName("MVP")->AsMatrix();
    _fxLightPosWorldTerrain = _effects->getTerrainEffect()->GetVariableByName("lightPosWorld")->AsVector();
    _fxNoiseTexture = _effects->getTerrainEffect()->GetVariableByName("noiseTexture")->AsShaderResource();
    _fxTerrainTexture = _effects->getTerrainEffect()->GetVariableByName("terrainTexture")->AsShaderResource();
    _fxTerrainTexture2 = _effects->getTerrainEffect()->GetVariableByName("terrainTexture2")->AsShaderResource();
    _fxCamPosWorldTerrain = _effects->getTerrainEffect()->GetVariableByName("camPosWorld")->AsVector();
    _fxTerrainNormals = _effects->getTerrainEffect()->GetVariableByName("terrainNormals")->AsShaderResource();
    _fxTerrainNormals2 = _effects->getTerrainEffect()->GetVariableByName("terrainNormals2")->AsShaderResource();
    _fxPtrFrequ = _effects->getTerrainEffect()->GetVariableByName("noiseFrequ")->AsScalar();
    _fxPtrHeightScale = _effects->getTerrainEffect()->GetVariableByName("heightScale")->AsScalar();
    _fxPtrNumOctaves = _effects->getTerrainEffect()->GetVariableByName("numOctaves")->AsScalar();
    _fxAmpScale = _effects->getTerrainEffect()->GetVariableByName("ampScale")->AsScalar();
    _fxFrequencyScale = _effects->getTerrainEffect()->GetVariableByName("frequencyScale")->AsScalar();
    _fxUvScaleDetails = _effects->getTerrainEffect()->GetVariableByName("uvScaleDetails")->AsScalar();
    _fxTerrainSize = _effects->getTerrainEffect()->GetVariableByName("terrainSize")->AsScalar();
    _fxMaxTessFactor = _effects->getTerrainEffect()->GetVariableByName("maxTessFactor")->AsScalar();
    _fxMaxTessDistance = _effects->getTerrainEffect()->GetVariableByName("maxTessDistance")->AsScalar();

    setSSRBlendWeight(_ssrBlendWeight);
  }

  void RenderingSystemDX11::present() const
  {
    _swapChain->Present(static_cast<unsigned>(_settings._vsync), 0);
  }

  CComPtr<ID3D11Device> RenderingSystemDX11::getDevice() const
  {
    return _device;
  }

  CComPtr<ID3D11DeviceContext> RenderingSystemDX11::getContext() const
  {
    return _context;
  }

  CComPtr<IDXGIAdapter> RenderingSystemDX11::getAdapter() const
  {
    return _dxgiAdapter;
  }

  void RenderingSystemDX11::printQuadtree() const
  {
    _quadtree->print();
  }

  void RenderingSystemDX11::buildQuadtree()
  {
    Timing timing;
    _quadtree = std::make_unique<Quadtree<DX11StaticModelRenderable>>(_sceneMin, _sceneMax);
    _quadtree->setDetailCullingParams(_settings._detailCullingParams);
    for (const auto& r : _staticModelRenderables) {
      _quadtree->insert(r.second.get());
    }
    std::cout << "Quadtree construction took " << timing << std::endl,
    std::cout << "Quadtree nodes:" << _quadtree->getAllNodes().size() << std::endl;
  }

  void RenderingSystemDX11::setSettings(const Settings& settings)
  {
    _settings = settings;
    unsigned flag = 0;
    if (_settings._lensflareEnabled) {
      flag |= Effects::CompositeRenderFlags::Lensflare;
    }
    if (_settings._depthOfFieldEnabled) {
      flag |= Effects::CompositeRenderFlags::DepthOfField;
    }
    if (_settings._motionBlurEnabled) {
      flag |= Effects::CompositeRenderFlags::MotionBlur;
    }
    if (_settings._lightVolumesEnabled) {
      flag |= Effects::CompositeRenderFlags::LightVolumes;
    }
    _compositePass = _effects->getCompositeTechnique(static_cast<Effects::CompositeRenderFlags>(flag))->GetPassByIndex(0);
    HR(_fxDepthOfFieldDistances->SetFloatVector(settings._depthOfFieldDistances.ptr()));
    HR(_fxSkycolor->SetFloatVector(settings._skyColor.ptr()));
    HR(_fxBrightScale->SetFloat(settings._brightScale));
    HR(_fxBrightBias->SetFloat(settings._brightBias));
    HR(_fxExposure->SetFloat(settings._exposure));
    HR(_fxssrSteps->SetInt(settings._ssrSteps));
    HR(_fxssrRayLenScale->SetFloat(settings._ssrRayLenScale));
    HR(_fxssrMinRayLen->SetFloat(settings._ssrMinRayLen));

    D3D11_RASTERIZER_DESC rast_desc = {};
    rast_desc.AntialiasedLineEnable = false;
    rast_desc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
    rast_desc.DepthBiasClamp = 0.f;
    rast_desc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    rast_desc.FrontCounterClockwise = true;
    rast_desc.DepthBias = settings._smDepthBias;
    rast_desc.DepthClipEnable = false;
    rast_desc.SlopeScaledDepthBias = settings._smSlopeScaledDepthBias;
    _rastStateShadowMap = nullptr;
    HR(_device->CreateRasterizerState(&rast_desc, &_rastStateShadowMap));

    if (_quadtree) {
      _quadtree->setDetailCullingParams(settings._detailCullingParams);
    }

    initAdditionalRenderTargets();
  }

  const Settings& RenderingSystemDX11::getSettings() const
  {
    return _settings;
  }

  const Vec3f & RenderingSystemDX11::getSceneMin() const
  {
    return _sceneMin;
  }

  const Vec3f & RenderingSystemDX11::getSceneMax() const
  {
    return _sceneMax;
  }

  RenderingSystemDX11::RTT::RTT(const Vec2u& size, RenderingSystemDX11* rs, unsigned mip_levels, bool one_channel)
  {
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.ArraySize = 1;
    tex_desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
    tex_desc.Format = one_channel ? DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
    tex_desc.Height = size[1];
    tex_desc.MipLevels = mip_levels;
    if (mip_levels > 1) {
      tex_desc.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
    tex_desc.Width = size[0];

    CComPtr<ID3D11Texture2D> tex;
    HR(rs->_device->CreateTexture2D(&tex_desc, nullptr, &tex));

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = tex_desc.Format;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
    HR(rs->_device->CreateRenderTargetView(tex, &rtv_desc, &_rtv));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
    HR(rs->_device->CreateShaderResourceView(tex, &srv_desc, &_srv));

    _viewport.Height = static_cast<float>(size[1]);
    _viewport.MaxDepth = D3D11_MAX_DEPTH;
    _viewport.MinDepth = D3D11_MIN_DEPTH;
    _viewport.TopLeftX = 0.f;
    _viewport.TopLeftY = 0.f;
    _viewport.Width = static_cast<float>(size[0]);
  }

  void RenderingSystemDX11::initAdditionalRenderTargets()
  {
    if (_lightingBuffer) { // Make sure the lighting buffer is already initialized
      _depthStencilSrvCopy = nullptr;
      if (_settings._ssrEnabled) {
        CComPtr<ID3D11Texture2D> depth_stencil_tex_copy;
        HR(_device->CreateTexture2D(&_dsTextureDesc, nullptr, &depth_stencil_tex_copy));
        HR(_device->CreateShaderResourceView(depth_stencil_tex_copy, &_dsSrvDesc, &_depthStencilSrvCopy));
      }
      _lightingBufferCopy = _settings._ssrEnabled ? std::make_unique<RTT>(_viewportSize, this) : nullptr;
      _viewSpaceNormals = _settings._ssrEnabled ? std::make_unique<RTT>(_viewportSize, this) : nullptr;
      _viewSpaceZ = _settings._ssrEnabled ? std::make_unique<RTT>(_viewportSize, this, 1, true) : nullptr;
      _lensFlareChain = _settings._lensflareEnabled ? std::make_unique<DownsampleChain>(this, _settings._lensflareLevels, glm::uvec2(2u, 1u)) : nullptr;
      _dofBuffers[0] = _settings._depthOfFieldEnabled ? std::make_shared<RTT>(_viewportSize / 2u, this) : nullptr;
      _dofBuffers[1] = _settings._depthOfFieldEnabled ? std::make_shared<RTT>(_viewportSize / 2u, this) : nullptr;
    }
  }

  void RenderingSystemDX11::prepareRender(float time, float delta_time)
  {
    if (_quadtree == nullptr) {
      buildQuadtree();
    }
    // Filter out the noise
    _time = time;
    _deltaTime = delta_time;
    _deltaTimeFiltered = glm::mix(delta_time, _deltaTime, _lerpAlpha);
    _camPos = glm::mix(_camera->_pos, glm::vec3(_camPos), _lerpAlpha);
    _camEulerAngles = glm::slerp(glm::quat(_camera->_eulerAngles), _camEulerAngles, _lerpAlpha);
    _fpsFactor = _deltaTimeFiltered > 0.f ? glm::mix(1.f / _deltaTimeFiltered / _targetFPS, _fpsFactor, _lerpAlpha) : 1.f;

    _reflectiveSurfacesVisible = false;
    HR(_fxCamPosWorld->SetFloatVector(&_camera->_pos.r));
    HR(_fxMotionBlurStrength->SetFloat(_fpsFactor));
    _viewMatrix = _camera->getViewMatrix(_camPos, glm::eulerAngles(_camEulerAngles));
    _VP = _projectionMatrix * _viewMatrix;
    HR(_fxV->SetMatrixTranspose(_viewMatrix.ptr()));
    Mat4f v_inverse(inverse(_viewMatrix));
    HR(_fxVInverse->SetMatrixTranspose(v_inverse.ptr()));
    HR(_fxVP->SetMatrixTranspose(_VP.ptr()));
    auto vp_inverse = v_inverse * _PInverse;
    HR(_fxVPInverse->SetMatrixTranspose(vp_inverse.ptr()));
    HR(_fxVPBefore->SetMatrixTranspose(_VPBefore.ptr()));
    auto vp_inv_vp_before = _VPBefore * vp_inverse;
    HR(_fxVPInverseVPBefore->SetMatrixTranspose(vp_inv_vp_before.ptr()));
    HR(_fxTime->SetFloat(time));
    _VPBefore = _VP;
    auto view_matrix_light = _directionalLight._dl->getViewMatrix();
    _lightVP.clear();
    _directionalLight._dl->getViewProjectionMatrices(_aspectRatio, _near, _fov, v_inverse,
      view_matrix_light, static_cast<float>(_directionalLight._shadowMap->_size), _settings._smFrustumSplits, _lightVP, true);
    HR(_fxLightVPs->SetMatrixTransposeArray(_lightVP.front().ptr(), 0, static_cast<uint32_t>(_lightVP.size())));
    HR(_fxCascDistances->SetFloatArray(&_settings._smFrustumSplits[0], 0, _directionalLight._shadowMap->_numCascades));
    HR(_fxCamRightWorld->SetFloatVector(&_camera->_right.r));
    HR(_fxCamUpWorld->SetFloatVector(&_camera->_up.r));
    HR(_fxNumCascades->SetInt(_directionalLight._shadowMap->_numCascades));
    _context->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | (_settings._ssrEnabled ? D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL : 0), 1.f, 0);
    if (!_skyboxRenderable) {
      _context->ClearRenderTargetView(_lightingBuffer->_rtv, &_backgroundColor.r);
    }
    _context->OMSetDepthStencilState(_settings._ssrEnabled ? _dx11States->depthReadWriteStencilWrite() : _dx11States->depthReadWriteLessEqual(), 0);
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    _context->IASetInputLayout(_defaultInputLayout);
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

#if DX11_STATS
    _stats = {};
#endif
  }

  void RenderingSystemDX11::renderShadowMaps()
  {
    _context->RSSetState(_rastStateShadowMap);
    _context->RSSetViewports(1, &_directionalLight._shadowMap->_viewPort);
    ID3D11RenderTargetView* rtv[1] = { nullptr };
    _context->OMSetRenderTargets(1, rtv, _directionalLight._shadowMap->_dsv);
    _context->ClearDepthStencilView(_directionalLight._shadowMap->_dsv, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, 1.f, 0);
    UINT offset = 0, stride = sizeof(Vertex);
    auto visible_elements = _quadtree->getVisibleElementsWithDetailCulling<true, true>(_lightVP, _camPos);
#if DX11_STATS
    _stats._visibleModelsShadow += visible_elements.size();
#endif
    for (const auto& r : visible_elements) {
      //if (!_directionalLight._dl->aabbVisible<true>(light_mvps, *m_rdable._model->getAABB())) {
      if (!r->getAABBWorld()->isVisible<true, true>(_lightVP)) {
        continue;
      }
      std::vector<Mat4f> light_mvps;
      for (const auto& vp : _lightVP) {
        light_mvps.push_back(vp * r->getModelMatrix());
      }
      HR(_fxM->SetMatrixTranspose(r->getModelMatrix().ptr()));
      unsigned lod = r->getStaticModelRenderable()->selectLod(_camPos);
      const auto& model_data = r->getLodsModelData()[lod];
      _context->IASetVertexBuffers(0, 1, &model_data->_vertexBuffer.p, &stride, &offset);
      _context->IASetIndexBuffer(model_data->_indexBuffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
      int material_index = -1;
      for (const auto& mesh_desc : model_data->_meshDesc) {
        if (model_data->_meshDesc.size() > 1) { // The model is visible, only do fine-grained culling if it consists of more than one mesh.
          if (!mesh_desc._mesh->getAABB()->isVisible<true, true>(light_mvps)) {
            continue;
          }
        }
        int material_index_new = static_cast<int>(mesh_desc._materialIndex);
        if (material_index != material_index_new) {
          const auto& mat_desc = model_data->_materialDesc[material_index_new];
          const auto& material = r->getStaticModelRenderable()->getLods()[lod]->getMaterials()[material_index_new];
          HR(_fxAlphaMap->SetResource(mat_desc._alphaSrv));
          if (material->hasWindX() || material->hasWindZ()) {
            const auto& aabb = mesh_desc._mesh->getAABB();
            HR(_fxWindPivotMinY->SetFloat(aabb->getMin()[1]));
            HR(_fxWindPivotMaxY->SetFloat(aabb->getMax()[1]));
            HR(_fxWindStrength->SetFloat(material->getWindStrength()));
            HR(_fxWindFrequency->SetFloat(material->getWindFrequency()));
          }
          HR(mat_desc._shadowMapPass->Apply(0, _context));
        }
        _context->DrawIndexed(mesh_desc._numIndices, mesh_desc._indexOffset, mesh_desc._baseVertex);
#if DX11_STATS
        _stats._drawCallsShadow++;
        _stats._renderedTrianglesShadow += mesh_desc._numIndices / 3 * light_mvps.size();
#endif
        material_index = material_index_new;
      }
    }
  }

  void RenderingSystemDX11::renderScene()
  {
    _context->RSSetState(_dx11States->rastCullBackFrontCounterClockwiseFill());
    std::vector<ID3D11RenderTargetView*> rtvs = { _lightingBuffer->_rtv };
    if (_settings._ssrEnabled) {
      rtvs.push_back(_viewSpaceNormals->_rtv);
      rtvs.push_back(_viewSpaceZ->_rtv);
    }
    _context->OMSetRenderTargets(static_cast<unsigned>(rtvs.size()), &rtvs.front(), _depthStencilView);
    _context->RSSetViewports(1, &_viewPort);

    renderModels();
    renderTerrain();
    renderOpaqueParticles();
    renderTransparentParticles();
    HR(_fxShadowMap->SetResource(nullptr));
    renderSkybox();
    renderLightsources();
  }

  void RenderingSystemDX11::renderModels()
  {
    UINT offset = 0, stride = sizeof(Vertex);
    auto light_pos_view = _viewMatrix * Vec4f({ _directionalLight._dl->_pos[0], _directionalLight._dl->_pos[1], _directionalLight._dl->_pos[2], 1.f });
    HR(_fxLightPosView->SetFloatVector(&light_pos_view[0]));
    HR(_fxLightColor->SetFloatVector(_directionalLight._dl->_color.ptr()));
    HR(_fxShadowMap->SetResource(_directionalLight._shadowMap->_srv));
    //auto visible_elements = _quadtree->getVisibleElements<true>(_VP);
    auto visible_elements = _quadtree->getVisibleElementsWithDetailCulling<true, false>({ _VP }, _camPos);
#if DX11_STATS
    _stats._visibleModels += visible_elements.size();
#endif
    for (const auto& r : visible_elements) {
      unsigned lod = r->getStaticModelRenderable()->selectLod(_camPos);
      const auto& model_data = r->getLodsModelData()[lod];
      _context->IASetVertexBuffers(0, 1, &model_data->_vertexBuffer.p, &stride, &offset);
      _context->IASetIndexBuffer(model_data->_indexBuffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
      auto mvp = _VP * r->getModelMatrix();
      HR(_fxMVP->SetMatrixTranspose(mvp.ptr()));
      auto mv_inverse = inverse(_viewMatrix * r->getModelMatrix());
      HR(_fxMVInverseTranspose->SetMatrix(mv_inverse.ptr()));
      int material_index = -1;
      for (const auto& mesh_desc : model_data->_meshDesc) {
        if (model_data->_meshDesc.size() > 1) { // The model is visible, only do fine-grained culling if it consists of more than one mesh.
          if (!mesh_desc._mesh->getAABB()->isVisible<true, true>(mvp)) {
            continue;
          }
        }
        int material_index_new = static_cast<int>(mesh_desc._materialIndex);
        // Meshes are sorted by material, the following statement helps to reduce context switches
        if (material_index != material_index_new) {
          const auto& material = r->getStaticModelRenderable()->getLods()[lod]->getMaterials()[material_index_new];
          if (_settings._ssrEnabled) {
            _context->OMSetDepthStencilState(_dx11States->depthReadWriteStencilWrite(), material->isReflective());
            _reflectiveSurfacesVisible = _reflectiveSurfacesVisible || material->isReflective();
          }
          const auto& mat_desc = model_data->_materialDesc[material_index_new];
          HR(_fxDiffuseTex->SetResource(mat_desc._diffuseSrv));
          HR(_fxAlphaMap->SetResource(mat_desc._alphaSrv));
          HR(_fxNormalMap->SetResource(mat_desc._normalSrv));
          HR(_fxDiffuseColor->SetFloatVector(mat_desc._diffuseColor.ptr()));
          if (material->hasWindX() || material->hasWindZ()) {
            HR(_fxWindPivotMinY->SetFloat(mesh_desc._mesh->getAABB()->getMin()[1]));
            HR(_fxWindPivotMaxY->SetFloat(mesh_desc._mesh->getAABB()->getMax()[1]));
            HR(_fxWindStrength->SetFloat(material->getWindStrength()));
            HR(_fxWindFrequency->SetFloat(material->getWindFrequency()));
          }
          HR(mat_desc._pass->Apply(0, _context));
        }
        _context->DrawIndexed(mesh_desc._numIndices, mesh_desc._indexOffset, mesh_desc._baseVertex);
        material_index = material_index_new;
#if DX11_STATS
        _stats._drawCalls++;
        _stats._renderedTriangles += mesh_desc._numIndices / 3;
#endif
      }
    }
    HR(_fxShadowMap->SetResource(nullptr));
  }

  void RenderingSystemDX11::renderTerrain() const
  {
    if (_proceduralTerrainRenderable) {
      _context->IASetInputLayout(nullptr);
      HR(_fxLightPosWorldTerrain->SetFloatVector(_directionalLight._dl->_pos.ptr()));
      const auto& ptr = _proceduralTerrainRenderable->_ptr;
      const auto& terrain = _proceduralTerrainRenderable->_terrain;
      HR(_fxCamPosWorldTerrain->SetFloatVector(_camPos.ptr()));
      HR(_fxMVPTerrain->SetMatrixTranspose(_VP.ptr()));
      HR(_fxPtrFrequ->SetFloat(ptr->getFrequency()));
      HR(_fxPtrHeightScale->SetFloat(ptr->getHeightScale()));
      HR(_fxPtrNumOctaves->SetInt(ptr->getNumOctaves()));
      HR(_fxTerrainNormals->SetResource(_proceduralTerrainRenderable->_terrainNormalSrvs[0]));
      HR(_fxNoiseTexture->SetResource(_proceduralTerrainRenderable->_noiseSrv));
      HR(_fxAmpScale->SetFloat(ptr->getAmpScale()));
      HR(_fxFrequencyScale->SetFloat(ptr->getFrequencyScale()));
      HR(_fxUvScaleDetails->SetFloat(_proceduralTerrainRenderable->_terrain->getUVScaleDetails()));
      HR(_fxTerrainSize->SetInt(terrain->getSize()));
      HR(_fxMaxTessFactor->SetFloat(terrain->getMaxTessFactor()));
      HR(_fxMaxTessDistance->SetFloat(terrain->getMaxTessDistance()));
      int patches_per_dir = terrain->getSize() / 64;
      if (_settings._wireframe) {
        _context->RSSetState(_dx11States->rastWireFrame());
        HR(_effects->getTerrainEffect()->GetTechniqueByName("terrainTechWireframe")->GetPassByIndex(0)->Apply(0, _context));
      }
      else {
        HR(_effects->getTerrainEffect()->GetTechniqueByName("terrainTech")->GetPassByIndex(0)->Apply(0, _context));
      }

      _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
      _context->Draw(4 * patches_per_dir * patches_per_dir, 0);
      _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      _context->HSSetShader(nullptr, nullptr, 0);
      _context->DSSetShader(nullptr, nullptr, 0);
      _context->RSSetState(_dx11States->rastCullBackFrontCounterClockwiseFill());
      _context->IASetInputLayout(_defaultInputLayout);
    }
  }

  void RenderingSystemDX11::renderOpaqueParticles() const
  {
    if (_particleModelRenderables.size()) {
      unsigned offset = 0, stride = sizeof Vertex;
      for (const auto& p : _particleModelRenderables) {
        std::vector<Mat4f> particle_transforms;
        p.second._particleSystem->getParticleTransformations(particle_transforms);
        if (particle_transforms.size()) {
          const auto& p_renderable = p.second._modelRenderable;
          unsigned lod = 0;
          const auto& model_data = p_renderable->getLodsModelData()[lod];
          _context->IASetVertexBuffers(0, 1, &model_data->_vertexBuffer.p, &stride, &offset);
          _context->IASetIndexBuffer(model_data->_indexBuffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
          std::vector<Mat4f> matrices;
          auto mvp = _VP * Mat4f(p.first->getComponent<Transform>()->getModelMatrix());
          for (const auto& m : particle_transforms) {
            matrices.push_back(mvp * m);
          }
          HR(_fxMVPs->SetMatrixTransposeArray(matrices.front().ptr(), 0, static_cast<uint32_t>(matrices.size())));
          auto& diffuse_color = p_renderable->getStaticModelRenderable()->getLods()[lod]->getMaterials().front()->getDiffuseColor();
          HR(_fxDiffuseColor->SetFloatVector(diffuse_color.ptr()));
          HR(_lightParticlePass->Apply(0, _context));
          _context->DrawIndexedInstanced(model_data->_meshDesc.front()._numIndices, static_cast<unsigned>(matrices.size()), 0, 0, 0);
        }
      }
    }
  }

  void RenderingSystemDX11::renderTransparentParticles() const
  {
    if (_particleBillboardRenderables.size()) {
      _context->OMSetDepthStencilState(_commonStates->DepthRead(), 0);
      _context->OMSetBlendState(_commonStates->Additive(), nullptr, 0xffffffff);
      for (auto& p : _particleBillboardRenderables) {
        auto& particles = p.second._particleSystem->getParticles();
        if (particles.size()) {
          //  _context->IASetInputLayout(nullptr);
          _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
          auto transform = p.first->getComponent<Transform>();
          std::vector<Vec3f> particle_positions;
          std::vector<float> fades;
          for (const auto& p : particles) {
            particle_positions.push_back(Vec3f(p._position) * transform->getScale() + transform->getTranslation());
            fades.push_back(pow(glm::smoothstep(0.f, 0.2f, p._age) * (1.f - glm::smoothstep(0.8f, 1.f, p._age)), 2.f));
          }
          HR(_fxBillboardPosWorld->SetFloatVectorArray(particle_positions.front().ptr(), 0u, static_cast<unsigned>(particle_positions.size())));
          HR(_fxFades->SetFloatArray(&fades.front(), 0u, static_cast<uint32_t>(fades.size())));
          auto size = p.second._billboard->getSize();
          HR(_fxBillboardSize->SetFloatVector(&size.x));
          HR(_fxDiffuseTex->SetResource(p.second._srv));
          HR(_billboardPass->Apply(0, _context));
          _context->Draw(static_cast<unsigned>(particle_positions.size()), 0);
        }
      }
      _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }
  }

  void RenderingSystemDX11::renderSkybox() const
  {
    if (_skyboxRenderable) {
      _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      _context->RSSetState(_dx11States->rastCullFrontFrontCounterClockwiseFill());
      UINT offset = 0, stride = sizeof(Vertex);
      const auto& model_data = _skyboxRenderable->_modelData;
      _context->IASetVertexBuffers(0, 1, &model_data->_vertexBuffer.p, &stride, &offset);
      _context->IASetIndexBuffer(model_data->_indexBuffer, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 0);
      auto mvp = _projectionMatrix * Mat4f(glm::mat4(glm::mat3(_viewMatrix)));
      HR(_fxMVP->SetMatrixTranspose(mvp.ptr()));
      _effects->getEffect()->GetTechniqueByName("skyboxTech")->GetPassByIndex(0)->Apply(0, _context);
      _context->DrawIndexed(model_data->_meshDesc[0]._numIndices, 0, 0);
      _context->RSSetState(_dx11States->rastCullBackFrontCounterClockwiseFill());
    }
  }

  void RenderingSystemDX11::renderLightsources() const
  {
    _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    const auto& light_pos_world = _directionalLight._dl->_pos;
    auto pos_view = _viewMatrix * Vec4f({ light_pos_world[0], light_pos_world[1], light_pos_world[2], 1.f });
    if (pos_view[2] < 0.f) {
      auto pos = _projectionMatrix * pos_view;
      pos /= pos[3];
      HR(_fxQuadPos->SetFloatVector(&pos[0]));
      auto scale = glm::vec2(1.f, _aspectRatio) * 0.03f;
      HR(_fxQuadScale->SetFloatVector(&scale.r));
      HR(_lightSourcePass->Apply(0, _context));
      _context->Draw(4, 0);
    }
  }

  void RenderingSystemDX11::postProcessing() const
  {
    if (_settings._ssrEnabled) {
      ssr();
    }
    if (_settings._depthOfFieldEnabled) {
      gaussFilter(_dofBuffers, _lightingBuffer->_srv);
    }
    if (_settings._lensflareEnabled) {
      renderLensflares();
    }
  }

  void RenderingSystemDX11::renderLensflares() const
  {
    renderBrightPass(*_lensFlareChain->_buffers[0][0], _lightingBuffer->_srv);
    for (unsigned i = 0; i < _lensFlareChain->_buffers.size(); i++) {
      gaussFilter(_lensFlareChain->_buffers[i], i == 0 ? _lensFlareChain->_buffers[0][0]->_srv : _lensFlareChain->_buffers[i - 1][0]->_srv);
    }
    _context->OMSetBlendState(_dx11States->blendAdditiveColor(), nullptr, 0xffffffff);
    for (int i = static_cast<int>(_lensFlareChain->_buffers.size() - 1); i >= 1; i--) {
      copy(_lensFlareChain->_buffers[i][0]->_srv, _lensFlareChain->_buffers[i - 1][0]->_rtv, _lensFlareChain->_buffers[i - 1][0]->_viewport);
    }
    _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
  }

  void RenderingSystemDX11::ssr() const
  {
    if (_reflectiveSurfacesVisible) {
      glm::vec4 blend_factor(_ssrBlendWeight);
      _context->OMSetBlendState(_dx11States->blendNonPremultipliedBlendFactorColor(), &blend_factor.r, 0xffffffff);
      _context->OMSetDepthStencilState(_dx11States->depthNoneStencilReadEqual(), 1);
      _context->OMSetRenderTargets(1, &_lightingBuffer->_rtv.p, _depthStencilView);
      _context->RSSetViewports(1, &_viewPort);
      HR(_fxVsNormalsTexture->SetResource(_viewSpaceNormals->_srv));
      HR(_fxVsZTexture->SetResource(_viewSpaceZ->_srv));
      copy(_depthStencilSrv, _depthStencilSrvCopy);
      HR(_fxDepthTex->SetResource(_depthStencilSrvCopy));
      copy(_lightingBuffer->_srv, _lightingBufferCopy->_srv);
      HR(_fxLightingTex->SetResource(_lightingBufferCopy->_srv));
      HR(_ssrPass->Apply(0, _context));
      _context->Draw(4, 0);
      HR(_fxVsNormalsTexture->SetResource(nullptr));
      HR(_fxVsZTexture->SetResource(nullptr));
      HR(_ssrPass->Apply(0, _context));
      _context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    }
  }

  void RenderingSystemDX11::composite() const
  {
    /* for (unsigned int i = 0; i < _directionalLight._shadowMap->_numCascades; i++) {
       _context->OMSetRenderTargets(1, &_directionalLight._shadowMap->_minMaxRtvs[i][0].p, nullptr);
       _context->RSSetViewports(1, &_directionalLight._shadowMap->_minMaxViewports[i][0]);
       HR(_fxMinMaxTexture->SetResource(_directionalLight._shadowMap->_srv));
       glm::vec2 texel_size = 1.f / glm::vec2(_directionalLight._shadowMap->_minMaxViewports[i][0].Width, _directionalLight._shadowMap->_minMaxViewports[i][0].Height);
       HR(_fxTexelSize->SetFloatVector(&texel_size.r));
       HR(_fxminMaxArraySlice->SetFloat(static_cast<float>(i)));
       HR(_minMaxPass->Apply(0, _context));
       _context->Draw(4, 0);
       HR(_fxMinMaxTexture->SetResource(nullptr));
       HR(_minMaxPass->Apply(0, _context));
     }
     for (unsigned int i = 0; i < _directionalLight._shadowMap->_numCascades; i++) {
       for (unsigned int mip = 1; mip < _directionalLight._shadowMap->_mips; mip++) {
         _context->OMSetRenderTargets(1, &_directionalLight._shadowMap->_minMaxRtvs[i][mip].p, nullptr);
         _context->RSSetViewports(1, &_directionalLight._shadowMap->_minMaxViewports[i][mip]);
         HR(_fxMinMaxTexture2->SetResource(_directionalLight._shadowMap->_minMaxSrvs[i][mip - 1]));
         glm::vec2 texel_size = 1.f / glm::vec2(_directionalLight._shadowMap->_minMaxViewports[i][mip].Width, _directionalLight._shadowMap->_minMaxViewports[i][mip].Height);
         HR(_fxTexelSize->SetFloatVector(&texel_size.r));
         HR(_minMaxPass2->Apply(0, _context));
         _context->Draw(4, 0);
         HR(_fxMinMaxTexture2->SetResource(nullptr));
         HR(_minMaxPass2->Apply(0, _context));
       }
     }*/

    _context->OMSetRenderTargets(1, &_backBufferRtv.p, nullptr);
    _context->RSSetViewports(1, &_viewPort);
    HR(_fxLightingTex->SetResource(_lightingBuffer->_srv));
    HR(_fxDepthTex->SetResource(_depthStencilSrv));
    HR(_fxLensflareTexture->SetResource(_settings._lensflareEnabled ? _lensFlareChain->_buffers[0][0]->_srv : nullptr));
    HR(_fxDofTexture->SetResource(_settings._depthOfFieldEnabled ? _dofBuffers[0]->_srv : nullptr));
    HR(_fxShadowMap->SetResource(_directionalLight._shadowMap->_srv));
    // HR(_fxMinMaxTexture2->SetResource(_directionalLight._shadowMap->_minMaxSrvs[0].back()));
    HR(_compositePass->Apply(0, _context));
    _context->Draw(4, 0);
    HR(_fxLightingTex->SetResource(nullptr));
    HR(_fxDepthTex->SetResource(nullptr));
    HR(_fxLensflareTexture->SetResource(nullptr));
    HR(_fxDofTexture->SetResource(nullptr));
    HR(_fxShadowMap->SetResource(nullptr));
    HR(_compositePass->Apply(0, _context));
  }

  void RenderingSystemDX11::createTexture(LPCWSTR path, SrvPtr& srv, DirectX::TEX_FILTER_FLAGS tex_filter)
  {
    if (!PathFileExistsW(path)) {
      std::wcout << path << L" not found" << std::endl;
      return;
    }
    if (_textureCache.count(path)) {
      srv = _textureCache[path];
      return;
    }
    wchar_t ext[_MAX_EXT];
    _wsplitpath_s(path, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);
    DirectX::ScratchImage image;
    if (!_wcsicmp(ext, L".tga")) {
      HR(DirectX::LoadFromTGAFile(path, nullptr, image));
    }
    if (!_wcsicmp(ext, L".png") || !_wcsicmp(ext, L".jpg")) {
      HR(DirectX::LoadFromWICFile(path, 0, nullptr, image));
    }
    if (!_wcsicmp(ext, L".dds")) {
      HR(DirectX::LoadFromDDSFile(path, 0, nullptr, image));
      HR(DirectX::CreateShaderResourceView(_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &srv));
      return;
    }
    DirectX::ScratchImage mip_chain;
    HR(DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), tex_filter, 0, mip_chain));
    HR(DirectX::CreateShaderResourceView(_device, mip_chain.GetImages(), mip_chain.GetImageCount(), mip_chain.GetMetadata(), &srv));
    _textureCache[path] = srv;
  }

  void RenderingSystemDX11::gaussFilter(const PingPongBuffer& buffer, const SrvPtr& input) const
  {
    _context->RSSetViewports(1, &buffer[0]->_viewport);
    auto texel_size = Vec2f(1.f) / Vec2f({ buffer[0]->_viewport.Width, buffer[0]->_viewport.Height });
    HR(_fxTexelSize->SetFloatVector(texel_size.ptr()));

    HR(_fxBlurInputTex->SetResource(input));
    _context->OMSetRenderTargets(1, &buffer[1]->_rtv.p, nullptr);
    HR(_gaussPassHor->Apply(0, _context));
    _context->Draw(4, 0);
    HR(_fxBlurInputTex->SetResource(nullptr));
    HR(_gaussPassHor->Apply(0, _context));

    HR(_fxBlurInputTex->SetResource(buffer[1]->_srv));
    _context->OMSetRenderTargets(1, &buffer[0]->_rtv.p, nullptr);
    HR(_gaussPassVert->Apply(0, _context));
    _context->Draw(4, 0);
    HR(_fxBlurInputTex->SetResource(nullptr));
    HR(_gaussPassVert->Apply(0, _context));
  }


  void RenderingSystemDX11::renderBrightPass(const RTT& rtt, const SrvPtr& srv) const
  {
    HR(_fxLightingTex->SetResource(srv));
    _context->OMSetRenderTargets(1, &rtt._rtv.p, nullptr);
    _context->RSSetViewports(1, &rtt._viewport);
    HR(_brightPass->Apply(0, _context));
    _context->Draw(4, 0);
    HR(_fxLightingTex->SetResource(nullptr));
    HR(_brightPass->Apply(0, _context));
  }

  void RenderingSystemDX11::copy(const SrvPtr& from, const RtvPtr& to, const D3D11_VIEWPORT& vp) const
  {
    _context->OMSetRenderTargets(1, &to.p, nullptr);
    _context->RSSetViewports(1, &vp);
    HR(_fxTexToCopy->SetResource(from));
    HR(_copyPass->Apply(0, _context));
    _context->Draw(4, 0);
    HR(_fxTexToCopy->SetResource(nullptr));
    HR(_copyPass->Apply(0, _context));
  }

  void RenderingSystemDX11::copy(const SrvPtr & from, const SrvPtr & to) const
  {
    CComPtr<ID3D11Resource> src, dst;
    from->GetResource(&src);
    to->GetResource(&dst);
    _context->CopyResource(dst, src);
  }

  /* RenderingSystemDX11::DX11StaticModelRenderable::DX11StaticModelRenderable(const std::shared_ptr<Model>& model,
     const std::shared_ptr<Transform>& transform, RenderingSystemDX11* rs)
     : _model(model),
     _modelMatrix(transform->getModelMatrix()),
     _modelData(std::make_unique<ModelData>(model, rs))
   {
   }*/

  RenderingSystemDX11::ShadowMap::ShadowMap(RenderingSystemDX11* rs)
  {
    _viewPort.Width = static_cast<float>(_size);
    _viewPort.Height = static_cast<float>(_size);
    _viewPort.MaxDepth = D3D11_MAX_DEPTH;
    _viewPort.MinDepth = D3D11_MIN_DEPTH;
    _viewPort.TopLeftX = 0.f;
    _viewPort.TopLeftY = 0.f;

    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.ArraySize = _numCascades;
    tex_desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
    tex_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
    tex_desc.Height = _size;
    tex_desc.MipLevels = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
    tex_desc.Width = _size;
    CComPtr<ID3D11Texture2D> depth_tex;
    HR(rs->_device->CreateTexture2D(&tex_desc, nullptr, &depth_tex));

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srv_desc.Texture2DArray.ArraySize = tex_desc.ArraySize;
    srv_desc.Texture2DArray.MipLevels = tex_desc.MipLevels;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    HR(rs->_device->CreateShaderResourceView(depth_tex, &srv_desc, &_srv));

    //_dsv.resize(_numCascades);
   // for (unsigned int i = 0; i < _numCascades; i++) {
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
    dsv_desc.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_desc.Texture2DArray.ArraySize = tex_desc.ArraySize;
    dsv_desc.Texture2DArray.FirstArraySlice = 0;
    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    HR(rs->_device->CreateDepthStencilView(depth_tex, &dsv_desc, &_dsv));
    // }

     /*for (unsigned int i = 0; i < _numCascades; i++) {
       std::vector<RtvPtr> rtvs (_mips);
       std::vector<SrvPtr> srvs (_mips);
       std::vector <D3D11_VIEWPORT> viewports(_mips);
       auto size = _size / 2;
       for (unsigned int mip = 0; mip < _mips; mip++, size /= 2) {
         tex_desc.ArraySize = 1;
         tex_desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
         tex_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
         tex_desc.Height = size;
         tex_desc.Width = size;
         CComPtr<ID3D11Texture2D> min_max_tex;
         HR(rs->_device->CreateTexture2D(&tex_desc, nullptr, &min_max_tex));
         srv_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
         srv_desc.Texture2D.MipLevels = 1;
         srv_desc.Texture2D.MostDetailedMip = 0;
         srv_desc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
         HR(rs->_device->CreateShaderResourceView(min_max_tex, &srv_desc, &srvs[mip]));
         D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
         rtv_desc.Format = DXGI_FORMAT_R16G16_FLOAT;
         rtv_desc.Texture2D.MipSlice = 0;
         rtv_desc.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
         HR(rs->_device->CreateRenderTargetView(min_max_tex, &rtv_desc, &rtvs[mip]));
         D3D11_VIEWPORT vp = _viewPort;
         vp.Width = size;
         vp.Height = size;
         viewports[mip] = vp;
       }
       _minMaxRtvs.push_back(rtvs);
       _minMaxSrvs.push_back(srvs);
       _minMaxViewports.push_back(viewports);
     }*/
  }

  RenderingSystemDX11::DownsampleChain::DownsampleChain(RenderingSystemDX11 * rs, unsigned int levels, const Vec2u& divisor)
  {
    auto size = rs->_viewportSize / 2u;
    for (unsigned int i = 0; i < levels && size[0] > 0u && size[1] > 0u; i++, size /= divisor) {
      PingPongBuffer buf;
      buf[0] = std::make_shared<RTT>(size, rs);
      buf[1] = std::make_shared<RTT>(size, rs);
      _buffers.push_back(buf);
    }
  }
  RenderingSystemDX11::ModelData::ModelData(const std::shared_ptr<Model>& model, RenderingSystemDX11* rs)
  {
    std::vector<unsigned> indices;
    std::vector<Vertex> vertices;
    _meshDesc.resize(model->getMeshes().size());
    for (unsigned int i = 0; i < model->getMeshes().size(); i++) {
      auto m = model->getMeshes()[i];
      _meshDesc[i]._indexOffset = static_cast<unsigned>(indices.size());
      _meshDesc[i]._baseVertex = static_cast<unsigned>(vertices.size());
      _meshDesc[i]._numIndices = static_cast<unsigned>(m->getIndices().size());
      _meshDesc[i]._materialIndex = m->getMaterialIndex();
      _meshDesc[i]._mesh = m.get();
      indices.insert(indices.end(), m->getIndices().begin(), m->getIndices().end());
      vertices.insert(vertices.end(), m->getVertices().begin(), m->getVertices().end());
    }
    D3D11_BUFFER_DESC desc = {};
    desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
    desc.ByteWidth = static_cast<unsigned>(vertices.size() * sizeof(vertices.front()));
    desc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = &vertices.front();
    HR(rs->_device->CreateBuffer(&desc, &data, &_vertexBuffer));
    desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
    desc.ByteWidth = static_cast<unsigned>(indices.size() * sizeof(indices.front()));
    desc.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
    data.pSysMem = &indices.front();
    HR(rs->_device->CreateBuffer(&desc, &data, &_indexBuffer));

    auto materials = model->getMaterials();
    _materialDesc.resize(materials.size());

    for (unsigned int i = 0; i < materials.size(); i++) {
      unsigned mesh_flags = 0;
      unsigned shadow_flags = 0;
      _materialDesc[i]._diffuseColor = materials[i]->getDiffuseColor();
      auto diffuse_path = materials[i]->getDiffusePath();
      if (diffuse_path != "") {
        std::wstring diff_path(diffuse_path.begin(), diffuse_path.end());
        rs->createTexture(diff_path.c_str(), _materialDesc[i]._diffuseSrv, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_SRGB);
        mesh_flags |= Effects::MeshRenderFlags::Diffuse;
      }
      auto opacity_path = materials[i]->getOpacityPath();
      if (opacity_path != "") {
        std::wstring op_path(opacity_path.begin(), opacity_path.end());
        rs->createTexture(op_path.c_str(), _materialDesc[i]._alphaSrv, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT);
        mesh_flags |= Effects::MeshRenderFlags::Alpha;
        shadow_flags |= Effects::ShadowMapRenderFlags::ShadowAlpha;
      }
      auto normal_path = materials[i]->getNormalPath();
      if (normal_path != "") {
        std::wstring n_path(normal_path.begin(), normal_path.end());
        rs->createTexture(n_path.c_str(), _materialDesc[i]._normalSrv, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT);
        mesh_flags |= Effects::MeshRenderFlags::Normal;
      }
      if (materials[i]->hasWindX()) {
        mesh_flags |= Effects::MeshRenderFlags::WindX;
        shadow_flags |= Effects::ShadowMapRenderFlags::ShadowWindX;
      }
      if (materials[i]->hasWindZ()) {
        mesh_flags |= Effects::MeshRenderFlags::WindZ;
        shadow_flags |= Effects::ShadowMapRenderFlags::ShadowWindZ;
      }
      _materialDesc[i]._pass = rs->_effects->getMeshTechnique(static_cast<Effects::MeshRenderFlags>(mesh_flags))->GetPassByIndex(0);
      _materialDesc[i]._shadowMapPass = rs->_effects->getShadowMapTechnique(static_cast<Effects::ShadowMapRenderFlags>(shadow_flags))->GetPassByIndex(0);
    }
  }
  RenderingSystemDX11::DX11ProceduralTerrainRenderable::DX11ProceduralTerrainRenderable(const std::shared_ptr<TerrainNew>& terrain_new,
    const std::shared_ptr<ProceduralTerrainRenderable>& ptr, RenderingSystemDX11* rs) :
    _terrain(terrain_new),
    _ptr(ptr)
  {
    _terrainSrvs.resize(terrain_new->getAlbedoPaths().size());
    unsigned i = 0;
    for (const auto& path : terrain_new->getAlbedoPaths()) {
      std::wstring str(path.begin(), path.end());
      rs->createTexture(str.c_str(), _terrainSrvs[i++], DirectX::TEX_FILTER_FLAGS::TEX_FILTER_SRGB);
    }
    _terrainNormalSrvs.resize(terrain_new->getNormalPaths().size());
    i = 0;
    for (const auto& path : terrain_new->getNormalPaths()) {
      std::wstring str(path.begin(), path.end());
      rs->createTexture(str.c_str(), _terrainNormalSrvs[i++], DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT);
    }
    // rs->createTexture(L"assets/noise_tex.png", _noiseSrv, DirectX::TEX_FILTER_FLAGS::TEX_FILTER_DEFAULT);
    D3D11_TEXTURE2D_DESC tex_desc = {};
    tex_desc.ArraySize = 1;
    tex_desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
    tex_desc.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
    tex_desc.Height = ptr->getNoiseValues().rows;
    tex_desc.MipLevels = 1;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
    tex_desc.Width = ptr->getNoiseValues().cols;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = tex_desc.Format;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
    D3D11_SUBRESOURCE_DATA tex_data = {};
    tex_data.pSysMem = ptr->getNoiseValues().data;
    tex_data.SysMemPitch = ptr->getNoiseValues().cols * sizeof(float);
    //  tex_data.SysMemSlicePitch = ptr->getNoiseValues().rows * ptr->getNoiseValues().cols * sizeof(float);
    CComPtr<ID3D11Texture2D> tex;
    HR(rs->_device->CreateTexture2D(&tex_desc, &tex_data, &tex));
    HR(rs->_device->CreateShaderResourceView(tex, &srv_desc, &_noiseSrv));
  }
  RenderingSystemDX11::DX11SkyboxRenderable::DX11SkyboxRenderable(const std::shared_ptr<Model>& model, RenderingSystemDX11 * rs) : _model(model)
  {
    _modelData = std::make_unique<ModelData>(model, rs);
  }
  RenderingSystemDX11::DX11StaticModelRenderable::DX11StaticModelRenderable(const std::shared_ptr<StaticModelRenderable>& smr, RenderingSystemDX11* rs) : _smr(smr)
  {
    for (const auto& r : smr->getLods()) {
      if (!rs->_modelDataCache.count(r)) {
        rs->_modelDataCache[r] = std::make_shared<ModelData>(r, rs);
      }
      _lodsModelData.push_back(rs->_modelDataCache[r]);
    }
  }
  AABB* RenderingSystemDX11::DX11StaticModelRenderable::getAABBWorld() const
  {
    return _smr->getAABBWorld().get();
  }
  const Mat4f & RenderingSystemDX11::DX11StaticModelRenderable::getModelMatrix() const
  {
    return _smr->getModelMatrix();
  }
  const std::shared_ptr<StaticModelRenderable>& RenderingSystemDX11::DX11StaticModelRenderable::getStaticModelRenderable() const
  {
    return _smr;
  }
  const std::vector<std::shared_ptr<RenderingSystemDX11::ModelData>>& RenderingSystemDX11::DX11StaticModelRenderable::getLodsModelData() const
  {
    return _lodsModelData;
  }
}