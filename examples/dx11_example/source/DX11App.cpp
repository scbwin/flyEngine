#include <AssimpImporter.h>
#include <StaticModelRenderable.h>
#include <Renderables.h>
#include <DX11App.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <windowsx.h>
#include <Animation.h>
#include <dx11/DXUtils.h>
#include <physics/ParticleSystem.h>
#include <physics/PhysicsSystem.h>
#include <TerrainNew.h>
#include <NoiseGen.h>
#include <Entity.h>
#include <Mesh.h>
#include <Light.h>
#include <Billboard.h>
#include <random>
#include <LevelOfDetail.h>
#include <Engine.h>

DX11App::DX11App()
{
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 
  initWindow();
  initGame();
  ShowWindow(_window, SW_SHOWDEFAULT);
}

DX11App::~DX11App()
{
}

LRESULT DX11App::windowCallbackStatic(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
  return reinterpret_cast<DX11App*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->windowCallback(hwnd, msg, w_param, l_param);
}

LRESULT DX11App::windowCallback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
  if (TwEventWin(hwnd, msg, w_param, l_param)) {
    return 0;
  }
  switch (msg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_SIZE:
    _windowSize = glm::uvec2(LOWORD(l_param), HIWORD(l_param));
    _rs->onResize(_windowSize);
    TwWindowSize(_windowSize.x, _windowSize.y);
    break;
  case WM_MOUSEMOVE:
    onMouseMove(w_param, l_param);
    break;
  case WM_KEYUP:
    onKeyUp(w_param, l_param);
    break;
  }
  return DefWindowProc(hwnd, msg, w_param, l_param);
}

void DX11App::onKeyUp(WPARAM w_param, LPARAM l_param)
{
  if (w_param == 'I') {
    _gameTimer.stop();
    _rs->initEffects();
    _rs->onResize(_windowSize);
    _gameTimer.start();
    return;
  }
  auto create_animation = [this](const std::wstring& str, bool enabled) {
    _dbgString = (str + (enabled ? L" on" : L" off"));
    auto anim_entity = _engine->getEntityManager()->createEntity();
    std::weak_ptr<fly::Entity> anim_weak(anim_entity);
    auto animation = std::make_shared<fly::Animation>(2.f, _gameTimer.getTimeSeconds(), [this](float t) {
      _dbgStringAlpha = (t < 0.5f ? t * 2.f : (1.f - t) * 2.f);
    }, [anim_weak, this]() {
      _engine->getEntityManager()->removeEntity(anim_weak.lock());
    });
    anim_entity->addComponent(animation);
  };
  if (w_param == VK_ESCAPE) {
    if (_gameTimer.isStopped()) {
      _gameTimer.start();
    }
    else {
      _gameTimer.stop();
    }
  }
  else if (w_param == '1') {
    auto settings = _rs->getSettings();
    settings._lensflareEnabled = !settings._lensflareEnabled;
    _rs->setSettings(settings);
    create_animation(L"Bloom and lens flare", settings._lensflareEnabled);
  }
  else if (w_param == '2') {
    auto settings = _rs->getSettings();
    settings._depthOfFieldEnabled = !settings._depthOfFieldEnabled;
    _rs->setSettings(settings);
    create_animation(L"Depth of field", settings._depthOfFieldEnabled);
  }
  else if (w_param == '3') {
    auto settings = _rs->getSettings();
    settings._motionBlurEnabled = !settings._motionBlurEnabled;
    _rs->setSettings(settings);
    create_animation(L"Motion blur", settings._motionBlurEnabled);
  }
  else if (w_param == '4') {
    auto settings = _rs->getSettings();
    settings._ssrEnabled = !settings._ssrEnabled;
    _rs->setSettings(settings);
    create_animation(L"Screen space reflections", settings._ssrEnabled);
  }
  else if (w_param == '5') {
    auto settings = _rs->getSettings();
    settings._lightVolumesEnabled = !settings._lightVolumesEnabled;
    _rs->setSettings(settings);
    create_animation(L"Light rays", settings._lightVolumesEnabled);
  }
  else if (w_param == 'V') {
    auto settings = _rs->getSettings();
    settings._vsync = !settings._vsync;
    _rs->setSettings(settings);
    create_animation(L"Vsync", settings._vsync);
  }
}

void DX11App::onMouseMove(WPARAM w_param, LPARAM l_param)
{
  POINT pos = { GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param) };
  // glm::vec2 mouse_delta = glm::vec2(pos.x, pos.y) - glm::vec2(_windowSize) / 2.f;
  if ((w_param & MK_LBUTTON) != 0) {
    auto mouse_delta = glm::vec2(pos.x, pos.y) - _mousePosBefore;
    _camera->setEulerAngles(_camera->getEulerAngles() - glm::vec3(mouse_delta.x, mouse_delta.y, 0.f) * 0.005f);
  }
  _mousePosBefore = glm::vec2(pos.x, pos.y);
  /*  POINT center = { _windowSize.x / 2, _windowSize.y / 2 };
    ClientToScreen(_window, &center);
    SetCursorPos(center.x, center.y);*/
}

#if DX11_STATS
std::wstring DX11App::getStatsString()
{
  std::wstring str;
  auto stats = _rs->getStats();
  str += L"Visible models:" + formatNumber(stats._visibleModels) + L"\n";
  str += L"Draw calls:" + formatNumber(stats._drawCalls) + L"\n";
  str += L"Rendered triangles:" + formatNumber(stats._renderedTriangles) + L"\n";
  str += L"Visible models shadow map:" + formatNumber(stats._visibleModelsShadow) + L"\n";
  str += L"Draw calls shadow map:" + formatNumber(stats._drawCallsShadow) + L"\n";
  str += L"Rendered triangles shadow map:" + formatNumber(stats._renderedTrianglesShadow) + L"\n";
  return str;
}
std::wstring DX11App::formatNumber(unsigned number)
{
  //std::cout << number << std::endl;
  unsigned num = number;
  std::wstring ret;
  while (true) {
    unsigned remainder = num % 1000;
    num /= 1000;
    wchar_t rem_str[4];
    if (num == 0) {
      if (remainder >= 100) {
        swprintf_s(rem_str, L"%3u", remainder);
      }
      else if (remainder >= 10) {
        swprintf_s(rem_str, L"%2u", remainder);
      }
      else {
        swprintf_s(rem_str, L"%1u", remainder);
      }
    }
    else {
      swprintf_s(rem_str, L"%03u", remainder);
    }
    ret = rem_str + ret;
    if (num == 0) {
      break;
    }
    else {
      ret = L"," + ret;
    }
  }
  return ret;
}
#endif

void DX11App::drawDebugGUI()
{
  using namespace DirectX;
  _spriteBatch->Begin();
  XMVECTOR pos = DirectX::XMVectorSet(0, 0, 0, 1);
  float scale = 0.6f;
  _font->DrawString(_spriteBatch.get(), _debugString.c_str(), pos, DirectX::Colors::White, 0.f, g_XMZero, scale);
  XMVECTOR origin = _font->MeasureString(_debugString.c_str()) * XMVectorSet(0.f, scale, 0.f, 0.f);
  _spriteBatch->End();
  _spriteBatch->Begin(SpriteSortMode::SpriteSortMode_Deferred, _commonStates->NonPremultiplied());
  XMVECTORF32 col = { { { 1.f, 1.f, 1.f, _dbgStringAlpha } } };
  _font->DrawString(_spriteBatch.get(), _dbgString.c_str(), origin, col, 0.f, g_XMZero, scale);
  _spriteBatch->End();
}


void DX11App::initWindow()
{
  HINSTANCE h_instance = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));
  WNDCLASS wc = {};
  wc.lpfnWndProc = windowCallbackStatic;
  wc.hInstance = h_instance;
  wc.lpszClassName = "MyDX11Window";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  RegisterClass(&wc);
  _window = CreateWindowEx(0, wc.lpszClassName, wc.lpszClassName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, h_instance, nullptr);
  assert(_window);
  SetWindowLongPtr(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void DX11App::initGame()
{
#if !SPONZA
  int grid_size = 32 * 4;
  int image_size = 1024 * 4;
  int num_cells = image_size / grid_size;
  auto noise_gen = fly::NoiseGen(grid_size, true);
  cv::Mat noise_values(image_size, image_size, CV_32FC1);
  for (int x = 0; x < image_size; x++) {
    for (int y = 0; y < image_size; y++) {
      noise_values.at<float>(y, x) = noise_gen.getPerlin(glm::vec2(x, y) / static_cast<float>(num_cells)) * 0.5f + 0.5f;
    }
  }
  //cv::imshow("noise", noise_values);
#endif

  _engine = std::make_unique<fly::Engine>();
  std::array<fly::Vec2f, 2> quadtree_min_max = { fly::Vec2f(-512.f), fly::Vec2f(512.f) };
  _rs = std::make_shared<fly::RenderingSystemDX11>(_window, quadtree_min_max);
  _engine->addSystem(_rs);
  _engine->addSystem(std::make_shared<fly::AnimationSystem>());
  _engine->addSystem(std::make_shared<fly::PhysicsSystem>());

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(0.f, 1.f);
  std::uniform_real_distribution<float> sphere_scale_dist(0.2f, 1.f);

  std::shared_ptr<fly::IImporter> importer = std::make_unique<fly::AssimpImporter>();
  std::vector<std::shared_ptr<fly::Model>> sphere_lods = { importer->loadModel("assets/sphere_lod0.obj"),
    importer->loadModel("assets/sphere_lod1.obj"), importer->loadModel("assets/sphere_lod2.obj") };
#if SPONZA
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  sponza_model->getMaterials()[16]->setHasWindZ(true, 12.f, 0.005f);
  sponza_model->getMaterials()[17]->setHasWindZ(true, 12.f, 0.005f);
  sponza_model->getMaterials()[18]->setHasWindZ(true, 12.f, 0.005f);
  sponza_model->getMaterials()[19]->setHasWindZ(true, 30.f, 0.005f);
  sponza_model->getMaterials()[20]->setHasWindZ(true, 30.f, 0.005f);
  sponza_model->getMaterials()[21]->setHasWindZ(true, 30.f, 0.005f);
  auto new_material = std::make_shared<fly::Material>(*sponza_model->getMaterials()[20]);
  new_material->setHasWindZ(false, 0.f, 0.f);
  new_material->setHasWindX(true, 250.f, 0.005f);
  auto materials_new = sponza_model->getMaterials();
  materials_new.push_back(new_material);
  sponza_model->setMaterials(materials_new);
  sponza_model->getMeshes()[sponza_model->getMeshes().size() - 28]->setMaterialIndex(sponza_model->getMaterials().size() - 1);
  sponza_model->getMaterials()[10]->setIsReflective(true);
  sponza_model->sortMeshesByMaterial();
  auto sponza_lods = fly::LevelOfDetail().generateLODsWithDetailCulling(sponza_model, 7);
#if SPONZA_LARGE
  int width = 100;
  int height = 100;
  float extents = 512.f * 10.f;
  for (unsigned x = 0; x < width; x++) {
    for (unsigned y = 0; y < height; y++) {
#endif
      auto sponza_entity = _engine->getEntityManager()->createEntity();
#if SPONZA_LARGE
      fly::Vec2f uv = fly::Vec2f({ static_cast<float>(x), static_cast<float>(y) }) / fly::Vec2f({ static_cast<float>(width), static_cast<float>(height) });
      if (x > 0 && y > 0) {
        uv += (dist(gen) * 2.f - 1.f) * 0.003f;
      }
      sponza_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(uv[0] * extents, 0.f, uv[1] * extents), glm::vec3(0.01f)));
#else
      sponza_entity->addComponent(std::make_shared<fly::Transform>(fly::Vec3f(), fly::Vec3f(0.01f)));
#endif
      sponza_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(sponza_lods, sponza_entity->getComponent<fly::Transform>(), 60.f));
#if SPONZA_LARGE
    }
  }
#endif
#if SPONZA_LARGE
  std::vector<std::vector<std::shared_ptr<fly::Model>>> modified_sphere_models;
  // Generate new sphere models with color variations
  for (unsigned i = 0; i < 100; i++) {
    std::vector<std::shared_ptr<fly::Model>> new_lods;
    fly::Vec3f new_color({ dist(gen), dist(gen), dist(gen) });
    for (const auto& s : sphere_lods) {
      auto s_new = std::make_shared<fly::Model>(*s); // Copy sphere model
      auto materials_copy = s->copyMaterials();
      for (const auto& m : materials_copy) {
        m->setDiffuseColor(new_color);
      }
      s_new->setMaterials(materials_copy);
      new_lods.push_back(s_new);
    }
    modified_sphere_models.push_back(new_lods);
  }
  width = 1500;
  height = 1500;
  for (unsigned x = 0; x < width; x++) {
    for (unsigned y = 0; y < height; y++) {
      auto sphere_entity = _engine->getEntityManager()->createEntity();
      unsigned index = glm::clamp(dist(gen) * modified_sphere_models.size(), 0.f, static_cast<float>(modified_sphere_models.size() - 1));
      const auto& selected_lods = modified_sphere_models[index];
      fly::Vec2f uv = fly::Vec2f({ static_cast<float>(x), static_cast<float>(y) }) / fly::Vec2f({ static_cast<float>(width), static_cast<float>(height) });
      sphere_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(uv[0] * extents, dist(gen) * 75.f, uv[1] * extents), glm::vec3(sphere_scale_dist(gen), sphere_scale_dist(gen), sphere_scale_dist(gen))));
      sphere_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(selected_lods, sphere_entity->getComponent<fly::Transform>(), 10.f));
    }
  }

#endif

  /*auto spark_model = importer->loadModel("assets/spark_particle.obj");
  spark_model->getMaterials().front().setDiffuseColor(glm::vec3(1.f, 0.988f, 0.721f) * 5.f);
  std::vector<glm::vec3> particle_positions = { glm::vec3(-6.215827, 1.293194, -2.191585), glm::vec3(-6.187380, 1.302527, 1.442227), glm::vec3(4.897683, 1.288674, -2.196014), glm::vec3(4.864175, 1.298548, 1.441392) };
  fly::ParticleSystem::ParticleSystemDesc desc = {};
  desc._emitInterval = 0.05f;
  desc._gravity = glm::vec3(0.f, -1.f, 0.f);
  desc._impulseProbability = 0.5f;
  desc._impulseStrength = 4.f;
  desc._initialVelocity = glm::vec3(0.f, 32.f, 0.f);
  desc._initialVelocityRandomScale = glm::vec3(12.f, 0.f, 12.f);
  desc._lifeTime = 1.f;
  desc._mass = 0.005f;
  desc._maxParticles = 50;
  desc._randomImpulses = true;
  for (auto& p : particle_positions) {
    auto ent = _engine->getEntityManager()->createEntity();
    ent->addComponent(spark_model);
    ent->addComponent(std::make_shared<fly::ParticleSystem>(desc));
    ent->addComponent(std::make_shared<fly::Transform>(p, glm::vec3(0.015f)));
  }

  desc._emitInterval = 0.15f;
  desc._gravity = glm::vec3(0.f, 0.5f, 0.f);
  desc._initialVelocity = glm::vec3(0.f, 1.f, 0.f);
  desc._initialVelocityRandomScale = glm::vec3(10.f, 0.f, 10.f);
  desc._lifeTime = 4.f;
  desc._mass = 0.2f;
  desc._maxParticles = 500;
  desc._randomImpulses = false;
  for (auto& p : particle_positions) {
    auto fire_particle_system = _engine->getEntityManager()->createEntity();
    fire_particle_system->addComponent(std::make_shared<fly::Billboard>("assets/flames.png", 1.f, glm::vec2(0.2f)));
    fire_particle_system->addComponent(std::make_shared<fly::ParticleSystem>(desc));
    fire_particle_system->addComponent(std::make_shared<fly::Transform>(p, glm::vec3(0.02f)));
  }*/
#endif

#if SPONZA_LARGE
  auto plane_entity = _engine->getEntityManager()->createEntity();
  auto plane_model = importer->loadModel("assets/plane.obj");
  for (const auto& m : plane_model->getMaterials()) {
    m->setNormalPath("assets/ground_normals.png");
    m->setDiffuseColor(glm::vec3(0.654f, 0.501f, 0.164f));
  }
  std::vector<std::shared_ptr<fly::Mesh>> plane_meshes_new;
  auto scale = _rs->getSceneMax() - _rs->getSceneMin();
  for (auto mesh : plane_model->getMeshes()) {
    auto vertices = mesh->getVertices();
    for (auto& v : vertices) {
      v._uv *= fly::Vec2f({ scale[0], scale[2] });
    }
    plane_meshes_new.push_back(std::make_shared<fly::Mesh>(vertices, mesh->getIndices(), mesh->getMaterialIndex()));
  }
  plane_model = std::make_shared<fly::Model>(plane_meshes_new, plane_model->getMaterials());

  plane_entity->addComponent(std::make_shared<fly::Transform>(_rs->getSceneMin(), fly::Vec3f({ scale[0], 1.f, scale[2] })));
  std::vector<std::shared_ptr<fly::Model>> plane_lods = { plane_model };
  plane_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(plane_lods, plane_entity->getComponent<fly::Transform>(), 0.f));
#endif

  auto cam_entity = _engine->getEntityManager()->createEntity();
  _camera = std::make_shared<fly::Camera>(glm::vec3(0.f, 3.f, 0.f), glm::vec3(0.f));
  cam_entity->addComponent(_camera);
  _camController = std::make_unique<fly::CameraController>(_camera, _camSpeed);

  auto dl_entity = _engine->getEntityManager()->createEntity();
#if SPONZA
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(30.f, 1024.f, 0.f), glm::vec3(512.f * 5.f, 0.f, 512.f * 5.f));
#else 
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(8192.f, 1000.f, 8192.f), glm::vec3(0.f, 0.f, 2.f), csm_distances);
#endif
  dl_entity->addComponent(_dl);
  dl_entity->addComponent(std::shared_ptr<fly::Light>(_dl));

#if !SPONZA
  auto terrain_entity = _engine->getEntityManager()->createEntity();
  std::vector<std::string> paths = {};
  std::vector<std::string> normal_paths = { "assets/ground_normals.png" };
  auto terrain = std::make_shared<fly::TerrainNew>(16384, paths, normal_paths, 60.f);
  terrain_entity->addComponent(terrain);
  auto ptr = std::make_shared<fly::ProceduralTerrainRenderable>(0.000002f, glm::vec2(0.001f),
    fly::ProceduralTerrainRenderable::NoiseType::RidgedMultiFractal, 1000.f, 8, noise_values, 2.604f, 0.320f);
  terrain_entity->addComponent(ptr);
#endif

  auto skydome_entity = _engine->getEntityManager()->createEntity();
  // auto sphere_model = importer->loadModel("assets/sphere.obj");
  skydome_entity->addComponent(sphere_lods[0]);
  // skydome_entity->addComponent(std::make_shared<fly::Transform>());
  skydome_entity->addComponent(std::make_shared<fly::SkyboxRenderable>());

  _commonStates = std::make_unique<DirectX::CommonStates>(_rs->getDevice());
  _font = std::make_unique<DirectX::SpriteFont>(_rs->getDevice(), L"assets/courier_new.spritefont");
  _spriteBatch = std::make_unique<DirectX::SpriteBatch>(_rs->getContext());
  DXGI_ADAPTER_DESC adapter_desc = {};
  _rs->getAdapter()->GetDesc(&adapter_desc);
  _adapterString = adapter_desc.Description;
  ULONGLONG sys_mem_kb;
  GetPhysicallyInstalledSystemMemory(&sys_mem_kb);
  _adapterString += L"\n" + std::to_wstring(sys_mem_kb / 1024) + L" MB RAM";

  TwInit(TwGraphAPI::TW_DIRECT3D11, _rs->getDevice());
  const std::string bar_name = "Settings";
  auto bar = TwNewBar(bar_name.c_str());

#if !SPONZA
  TwAddVarCB(bar, "Noise frequency", TwType::TW_TYPE_FLOAT, TwSetPtrFrequCallback, TwGetPtrFrequCallback,
    ptr.get(), "step=0.00000003 Group=Terrain");
  TwAddVarCB(bar, "Height scale", TwType::TW_TYPE_FLOAT, TwSetPtrHeightScaleCallback, TwGetPtrHeightScaleCallback,
    ptr.get(), "Group=Terrain");
  TwAddVarCB(bar, "Num octaves", TwType::TW_TYPE_INT32, TwSetPtrOctavesCallback, TwGetPtrOctavesCallback,
    ptr.get(), "min=1 max=9 Group=Terrain");
  TwAddVarCB(bar, "Amp scale", TwType::TW_TYPE_FLOAT, TwSetAmpScaleCb, TwGetAmpScaleCb,
    ptr.get(), "step=0.001 Group=Terrain");
  TwAddVarCB(bar, "Frequency scale", TwType::TW_TYPE_FLOAT, TwSetFrequencyScaleCb, TwGetFrequencyScaleCb,
    ptr.get(), "step=0.001 Group=Terrain");
  TwAddVarCB(bar, "UV scale details", TwType::TW_TYPE_FLOAT, TwSetUVScaleDetailsCb, TwGetUVScaleDetailsCb, terrain.get(), "step=1 min=1 Group=Terrain");
  TwAddVarCB(bar, "Terrain size", TwType::TW_TYPE_INT32, TwSetTerrainSize, TwGetTerrainSize, terrain.get(), "step=1 min=8 max=16 Group=Terrain help='Size of the terrain in powers of 2'");
  TwAddVarCB(bar, "Max tesselation factor", TwType::TW_TYPE_FLOAT, TwSetMaxTessFactor, TwGetMaxTessFactor, terrain.get(), "min=0.0 max=6.0 step=0.01 Group=Terrain");
  TwAddVarCB(bar, "Max tesselation distance", TwType::TW_TYPE_FLOAT, TwSetMaxTessDistance, TwGetMaxTessDistance, terrain.get(), "min=0 max=16384 step=8 Group=Terrain");
#endif
  TwAddVarCB(bar, "Depth of Field", TW_TYPE_BOOLCPP, TwSetDofEnabled, TwGetDofEnabled, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "Depth of Field near", TW_TYPE_FLOAT, TwSetDofNear, TwGetDofNear, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "Depth of Field center", TW_TYPE_FLOAT, TwSetDofCenter, TwGetDofCenter, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "Depth of Field far", TW_TYPE_FLOAT, TwSetDofFar, TwGetDofFar, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "Lens flare", TwType::TW_TYPE_BOOLCPP, TwSetLensflareEnabled, TwGetLensflareEnabled, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "Bright scale", TW_TYPE_FLOAT, TwSetBrightScale, TwGetBrightScale, _rs.get(), "group=PostProcessing min=0 step=0.005");
  TwAddVarCB(bar, "Bright bias", TW_TYPE_FLOAT, TwSetBrightBias, TwGetBrightBias, _rs.get(), "group=PostProcessing min=0 step=0.005");
  TwAddVarCB(bar, "SM depth bias", TW_TYPE_INT32, TwSetSmDepthBias, TwGetSmDepthBias, _rs.get(), "group=Renderer min=0 max=2000000 step=350");
  TwAddVarCB(bar, "SM sloped scaled bias", TW_TYPE_FLOAT, TwSetSmSlopeScaledDepthBias, TwGetSmSlopeScaledDepthBias, _rs.get(), "group=Renderer min=0 max=1000 step = 0.05");
  TwAddVarCB(bar, "Detail culling error threshold", TW_TYPE_FLOAT, TwSetDetailCullingErrorThreshold, TwGetDetailCullingErrorThreshold, _camera.get(), "group=Renderer min=0 max=100 step = 0.01");
#if SPONZA
  TwAddVarCB(bar, "Screen space reflections (SSR)", TW_TYPE_BOOLCPP, TwSetSSR, TwGetSSR, _rs.get(), "group=PostProcessing");
  TwAddVarCB(bar, "SSR blend weight", TW_TYPE_FLOAT, TwSetSSRWeight, TwGetSSRWeight, _rs.get(), "group=PostProcessing min=0 max=1 step=0.0035");
  TwAddVarCB(bar, "SSR steps", TW_TYPE_INT32, TwSetSSRSteps, TwGetSSRSteps, _rs.get(), "group=PostProcessing min=0 max=128 step=1");
  TwAddVarCB(bar, "SSR MinRayLen", TW_TYPE_FLOAT, TwSetSSRMinRayLen, TwGetSSRMinRayLen, _rs.get(), "group=PostProcessing min=1.0 max=1024.0 step=0.25");
  TwAddVarCB(bar, "SSR RayLenScale", TW_TYPE_FLOAT, TwSetSSRRayLenScale, TwGetSSRRayLenScale, _rs.get(), "group=PostProcessing min=0.1 max=128.0 step=0.25");
#endif
  TwAddVarCB(bar, "Exposure", TwType::TW_TYPE_FLOAT, TwSetExposure, TwGetExposure, _rs.get(), "group=Renderer step=0.005");
  TwAddVarRW(bar, "Cam speed", TwType::TW_TYPE_FLOAT, &_camSpeed, "min=1 group=Renderer");
  TwAddVarCB(bar, "Wireframe", TwType::TW_TYPE_BOOLCPP, TwSetWireframeCallback, TwGetWireframeCallback, _rs.get(), "Group=Renderer");
  TwAddVarCB(bar, "Skycolor", TwType::TW_TYPE_COLOR3F, TwSetSkycolor, TwGetSkycolor, _rs.get(), "group=Renderer");
  TwAddVarRW(bar, "Show gui", TwType::TW_TYPE_BOOLCPP, &_showGUI, "group=Renderer");

#if SPONZA
  auto settings = _rs->getSettings();
  settings._depthOfFieldDistances = glm::vec3(-2000.f, 500.f, 10000.f);
  settings._exposure = 0.4f;
  _rs->setSettings(settings);
#endif

  std::cout << "initGame() finished" << std::endl;
}

void DX11App::handleInput()
{
  if (keyPressed('W')) {
    _camController->stepForward(_gameTimer.getDeltaTimeSeconds());
  }
  if (keyPressed('A')) {
    _camController->stepLeft(_gameTimer.getDeltaTimeSeconds());
  }
  if (keyPressed('S')) {
    _camController->stepBackward(_gameTimer.getDeltaTimeSeconds());
  }
  if (keyPressed('D')) {
    _camController->stepRight(_gameTimer.getDeltaTimeSeconds());
  }
  if (keyPressed(VK_SPACE)) {
    _camController->stepUp(_gameTimer.getDeltaTimeSeconds());
  }
  if (keyPressed('C')) {
    _camController->stepDown(_gameTimer.getDeltaTimeSeconds());
  }
  float light_speed = 10.f;
  if (keyPressed(VK_LEFT)) {
    _dl->_pos[2] -= _gameTimer.getDeltaTimeSeconds() * light_speed;
  }
  if (keyPressed(VK_RIGHT)) {
    _dl->_pos[2] += _gameTimer.getDeltaTimeSeconds() * light_speed;
  }
  if (keyPressed(VK_UP)) {
    _dl->_pos[0] += _gameTimer.getDeltaTimeSeconds() * light_speed;
  }
  if (keyPressed(VK_DOWN)) {
    _dl->_pos[0] -= _gameTimer.getDeltaTimeSeconds() * light_speed;
  }
}

bool DX11App::keyPressed(int key)
{
  return GetAsyncKeyState(key) & 0x8000;
}

float DX11App::getCamSpeed()
{
  float cam_speed = _camSpeed;
  if (keyPressed(VK_SHIFT)) {
    cam_speed *= _camSpeedAccFactor;
  }
  if (keyPressed(VK_CONTROL)) {
    cam_speed *= _camSpeedDecFactor;
  }
  return cam_speed;
}

void DX11App::TwSetPtrFrequCallback(const void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  ptr->setFrequency(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetPtrFrequCallback(void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  *(reinterpret_cast<float*>(value)) = ptr->getFrequency();
}

void DX11App::TwSetPtrHeightScaleCallback(const void* value, void* client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  ptr->setHeightScale(*reinterpret_cast<const float*>(value));
}
void DX11App::TwGetPtrHeightScaleCallback(void* value, void* client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  *(reinterpret_cast<float*>(value)) = ptr->getHeightScale();
}

void DX11App::TwSetPtrOctavesCallback(const void* value, void* client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  ptr->setNumOctaves(*reinterpret_cast<const int*>(value));
}
void DX11App::TwGetPtrOctavesCallback(void* value, void* client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  *(reinterpret_cast<int*>(value)) = ptr->getNumOctaves();
}

void DX11App::TwSetWireframeCallback(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._wireframe = !settings._wireframe;
  settings._depthOfFieldEnabled = false;
  settings._lensflareEnabled = false;
  rs->setSettings(settings);
}

void DX11App::TwGetWireframeCallback(void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  *(reinterpret_cast<bool*>(value)) = settings._wireframe;
}

void DX11App::TwSetAmpScaleCb(const void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  ptr->setAmpScale(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetAmpScaleCb(void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  *(reinterpret_cast<float*>(value)) = ptr->getAmpScale();
}

void DX11App::TwSetFrequencyScaleCb(const void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  ptr->setFrequencyScale(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetFrequencyScaleCb(void * value, void * client_data)
{
  auto ptr = reinterpret_cast<fly::ProceduralTerrainRenderable*>(client_data);
  *(reinterpret_cast<float*>(value)) = ptr->getFrequencyScale();
}

void DX11App::TwSetUVScaleDetailsCb(const void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  terrain->setUVScaleDetails(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetUVScaleDetailsCb(void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  *(reinterpret_cast<float*>(value)) = terrain->getUVScaleDetails();
}

void DX11App::TwSetDofNear(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._depthOfFieldDistances[0] = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetDofNear(void* value, void* client_data)
{
  auto settings = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings();
  *reinterpret_cast<float*>(value) = settings._depthOfFieldDistances[0];
}
void DX11App::TwSetDofCenter(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._depthOfFieldDistances[1] = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetDofCenter(void* value, void* client_data)
{
  auto settings = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings();
  *reinterpret_cast<float*>(value) = settings._depthOfFieldDistances[1];
}
void DX11App::TwSetDofFar(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._depthOfFieldDistances[2] = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetDofFar(void* value, void* client_data)
{
  auto settings = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings();
  *reinterpret_cast<float*>(value) = settings._depthOfFieldDistances[2];
}

void DX11App::TwSetTerrainSize(const void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  terrain->setSize(pow(2, *reinterpret_cast<const int*>(value)));
}

void DX11App::TwGetTerrainSize(void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  *reinterpret_cast<int*>(value) = log2(terrain->getSize());
}

void DX11App::TwSetMaxTessFactor(const void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  terrain->setMaxTessFactor(*reinterpret_cast<const float*>(value));
}

void  DX11App::TwGetMaxTessFactor(void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  *reinterpret_cast<float*>(value) = terrain->getMaxTessFactor();
}

void DX11App::TwSetMaxTessDistance(const void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  terrain->setMaxTessDistance(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetMaxTessDistance(void * value, void * client_data)
{
  auto terrain = reinterpret_cast<fly::TerrainNew*>(client_data);
  *reinterpret_cast<float*>(value) = terrain->getMaxTessDistance();
}

void DX11App::TwSetSkycolor(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  auto col = reinterpret_cast<const float*>(value);
  settings._skyColor = glm::vec3(col[0], col[1], col[2]);
  rs->setSettings(settings);
}

void DX11App::TwGetSkycolor(void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  auto col = reinterpret_cast<float*>(value);
  std::memcpy(col, settings._skyColor.ptr(), sizeof settings._skyColor);
}

void DX11App::TwSetBrightScale(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._brightScale = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetBrightScale(void * value, void * client_data)
{
  auto settings = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings();
  *reinterpret_cast<float*>(value) = settings._brightScale;
}

void DX11App::TwSetBrightBias(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._brightBias = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetBrightBias(void * value, void * client_data)
{
  auto settings = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings();
  *reinterpret_cast<float*>(value) = settings._brightBias;
}

void DX11App::TwSetDofEnabled(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._depthOfFieldEnabled = !settings._depthOfFieldEnabled;
  rs->setSettings(settings);
}

void DX11App::TwGetDofEnabled(void * value, void * client_data)
{
  *reinterpret_cast<bool*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._depthOfFieldEnabled;
}

void DX11App::TwSetLensflareEnabled(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._lensflareEnabled = !settings._lensflareEnabled;
  rs->setSettings(settings);
}

void DX11App::TwGetLensflareEnabled(void * value, void * client_data)
{
  *reinterpret_cast<bool*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._lensflareEnabled;
}

void DX11App::TwSetExposure(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._exposure = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetExposure(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._exposure;
}

void DX11App::TwSetSSR(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._ssrEnabled = *reinterpret_cast<const bool*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetSSR(void * value, void * client_data)
{
  *reinterpret_cast<bool*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._ssrEnabled;
}

void DX11App::TwSetSSRWeight(const void * value, void * client_data)
{
  reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->setSSRBlendWeight(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetSSRWeight(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSSRBlendWeight();
}

void DX11App::TwSetSSRSteps(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._ssrSteps = *reinterpret_cast<const int*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetSSRSteps(void* value, void* client_data)
{
  *reinterpret_cast<int*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._ssrSteps;
}
void DX11App::TwSetSSRMinRayLen(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._ssrMinRayLen = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetSSRMinRayLen(void* value, void* client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._ssrMinRayLen;
}
void DX11App::TwSetSSRRayLenScale(const void* value, void* client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._ssrRayLenScale = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}
void DX11App::TwGetSSRRayLenScale(void* value, void* client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._ssrRayLenScale;
}

void DX11App::TwSetSmDepthBias(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._smDepthBias = *reinterpret_cast<const int*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetSmDepthBias(void * value, void * client_data)
{
  *reinterpret_cast<int*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._smDepthBias;
}

void DX11App::TwSetSmSlopeScaledDepthBias(const void * value, void * client_data)
{
  auto rs = reinterpret_cast<fly::RenderingSystemDX11*>(client_data);
  auto settings = rs->getSettings();
  settings._smSlopeScaledDepthBias = *reinterpret_cast<const float*>(value);
  rs->setSettings(settings);
}

void DX11App::TwGetSmSlopeScaledDepthBias(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::RenderingSystemDX11*>(client_data)->getSettings()._smSlopeScaledDepthBias;
}

void DX11App::TwSetDetailCullingErrorThreshold(const void * value, void * client_data)
{
  auto camera = reinterpret_cast<fly::Camera*>(client_data);
  camera->setDetailCullingThreshold(*reinterpret_cast<const float*>(value));
}

void DX11App::TwGetDetailCullingErrorThreshold(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::Camera*>(client_data)->getDetailCullingThreshold();
}

int DX11App::execute()
{
  _gameTimer = fly::GameTimer();
  MSG msg = {};
  PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
  float measure = _gameTimer.getTotalTimeSeconds() + 1.f;
  unsigned fps = 0;
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else {
      _gameTimer.tick();
      handleInput();
      _engine->update(_gameTimer.getTimeSeconds(), _gameTimer.getDeltaTimeSeconds());
     // std::cout << _engine->getEntityManager()->getEntities().size() << std::endl;
      if (_showGUI) {
        drawDebugGUI();
      }
      TwDraw();
      _rs->present();
      fps++;
      if (_gameTimer.getTotalTimeSeconds() >= measure) {
        _debugString = std::to_wstring(fps) + L" FPS" + L"\n" + _adapterString;
#if DX11_STATS
        _debugString += L"\n" + getStatsString();
#endif
        fps = 0;
        measure = _gameTimer.getTotalTimeSeconds() + 1.f;
      }
    }
  }
  TwTerminate();
  return 0;
}
