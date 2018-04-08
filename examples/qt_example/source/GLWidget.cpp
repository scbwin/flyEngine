#include <opengl/OpenGLAPI.h>
#include <GLWidget.h>
#include <AssimpImporter.h>
#include <Leakcheck.h>
#include <renderer/AbstractRenderer.h>
#include <iostream>
#include <Engine.h>
#include <Transform.h>
#include <GameTimer.h>
#include <Camera.h>
#include <CameraController.h>
#include <AntTweakBar.h>
#include <LevelOfDetail.h>
#include <StaticMeshRenderable.h>

GLWidget::GLWidget()
{
  _engine = std::make_unique<fly::Engine>();
  _gameTimer = std::make_unique<fly::GameTimer>();
  setMouseTracking(true);
}

GLWidget::~GLWidget()
{
  TwTerminate();
}

void GLWidget::initializeGL()
{
  _renderer = std::make_shared<fly::AbstractRenderer<fly::OpenGLAPI>>();
  _engine->addSystem(_renderer);
  initGame();
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
  TwInit(TwGraphAPI::TW_OPENGL_CORE, nullptr);
  _bar = TwNewBar("Stats");
  TwAddButton(_bar, _fpsButtonName, nullptr, nullptr, nullptr);

  auto debug_bar = TwNewBar("Debug");
  TwAddVarCB(debug_bar, "Quadtree AABBs", TwType::TW_TYPE_BOOLCPP, cbSetDebugQuadtreeAABBs, cbGetDebugQuadtreeAABBs, &_renderer, nullptr);
  TwAddVarCB(debug_bar, "Object AABBs", TwType::TW_TYPE_BOOLCPP, cbSetDebugObjectAABBs, cbGetDebugObjectAABBs, &_renderer, nullptr);
  TwAddButton(debug_bar, "Reload shaders", cbReloadShaders, &_renderer, nullptr);

  auto settings_bar = TwNewBar("Settings");
  TwAddVarCB(settings_bar, "Group by material", TwType::TW_TYPE_BOOLCPP, cbSetSortModeMaterial, cbGetSortModeMaterial, &_renderer, nullptr);
  TwAddVarCB(settings_bar, "Group by shader and material", TwType::TW_TYPE_BOOLCPP, cbSetSortModeShaderMaterial, cbGetSortModeShaderMaterial, &_renderer, nullptr);
  TwAddVarCB(settings_bar, "Light intensity", TwType::TW_TYPE_COLOR3F, cbSetLightIntensity, cbGetLightIntensity, &_dl, nullptr);
  TwAddVarCB(settings_bar, "Post processing", TwType::TW_TYPE_BOOLCPP, cbSetPostProcessing, cbGetPostProcessing, &_renderer, nullptr);
  TwAddVarCB(settings_bar, "Sponza specular", TwType::TW_TYPE_FLOAT, cbSetSpec, cbGetSpec, &_sponzaModel, "step=0.2");
  TwAddVarCB(settings_bar, "Shadows", TwType::TW_TYPE_BOOLCPP, cbSetShadows, cbGetShadows, &_renderer, nullptr);

  TwSetTopBar(_bar);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i( width, height ));
  TwWindowSize(width, height);
}

void GLWidget::paintGL()
{
  _gameTimer->tick();
  _renderer->setDefaultRendertarget(defaultFramebufferObject());
  _engine->update(_gameTimer->getTimeSeconds(), _gameTimer->getDeltaTimeSeconds());
  if (contains<int>(_keysPressed, 'W')) {
    _camController->stepForward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'A')) {
    _camController->stepLeft(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'S')) {
    _camController->stepBackward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'D')) {
    _camController->stepRight(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'C')) {
    _camController->stepDown(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, Qt::Key::Key_Space)) {
    _camController->stepUp(_gameTimer->getDeltaTimeSeconds());
  }
  _fps++;
  if (_gameTimer->getTotalTimeSeconds() >= _measure) {
    _measure = _gameTimer->getTotalTimeSeconds() + 1.f;
    std::string fps_label_str = std::to_string(_fps) + " FPS";
    TwSetParam(_bar, _fpsButtonName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, fps_label_str.c_str());
    _fps = 0;
  }
  TwDraw();
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
}

void GLWidget::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (!TwMouseButton(TwMouseAction::TW_MOUSE_PRESSED, TwMouseButtonID::TW_MOUSE_LEFT)) {
      _camController->mousePress(fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
    }
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (_camController->isPressed()) {
    _camController->mouseMove(fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
  }
  else {
    TwMouseMotion(e->localPos().x(), e->localPos().y());
  }
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (_camController->isPressed()) {
      _camController->mouseRelease();
    }
    else {
      TwMouseButton(TwMouseAction::TW_MOUSE_RELEASED, TwMouseButtonID::TW_MOUSE_LEFT);
    }
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}
void GLWidget::cbSetDebugQuadtreeAABBs(const void * value, void * clientData)
{
  fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
  settings._debugQuadtreeNodeAABBs = *reinterpret_cast<const bool*>(value);
  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
}
void GLWidget::cbGetDebugQuadtreeAABBs(void * value, void * clientData)
{
  *reinterpret_cast<bool*>(value) =  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._debugQuadtreeNodeAABBs;
}
void GLWidget::cbSetDebugObjectAABBs(const void * value, void * clientData)
{
  fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
  settings._debugObjectAABBs = *reinterpret_cast<const bool*>(value);
  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
}
void GLWidget::cbGetDebugObjectAABBs(void * value, void * clientData)
{
  *reinterpret_cast<bool*>(value) = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._debugObjectAABBs;
}
void GLWidget::cbSetSortModeMaterial(const void * value, void * clientData)
{
  if (*reinterpret_cast<const bool*>(value)) {
    fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
    settings._dlSortMode = fly::DisplayListSortMode::MATERIAL;
    (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
  }
}
void GLWidget::cbGetSortModeMaterial(void * value, void* clientData)
{
  *reinterpret_cast<bool*>(value) = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._dlSortMode == fly::DisplayListSortMode::MATERIAL;
}
void GLWidget::cbGetSortModeShaderMaterial(void * value, void* clientData)
{
  *reinterpret_cast<bool*>(value) = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._dlSortMode == fly::DisplayListSortMode::SHADER_AND_MATERIAL;
}
void GLWidget::cbSetLightIntensity(const void * value, void * clientData)
{
  (*reinterpret_cast<std::shared_ptr<fly::DirectionalLight>*>(clientData))->setIntensity(*reinterpret_cast<const fly::Vec3f*>(value));
}
void GLWidget::cbGetLightIntensity(void * value, void * clientData)
{
  *reinterpret_cast<fly::Vec3f*>(value) = (*reinterpret_cast<std::shared_ptr<fly::DirectionalLight>*>(clientData))->getIntensity();
}
void GLWidget::cbSetPostProcessing(const void * value, void * clientData)
{
  fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
  settings._postProcessing = *reinterpret_cast<const bool*>(value);
  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
}
void GLWidget::cbGetPostProcessing(void * value, void * clientData)
{
  *reinterpret_cast<bool*>(value) = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._postProcessing;
}
void GLWidget::cbSetSpec(const void * value, void * clientData)
{
  auto sponza_model = *reinterpret_cast<std::shared_ptr<fly::Model>*>(clientData);
  for (const auto& m : sponza_model->getMaterials()) {
    m->setSpecularExponent(*reinterpret_cast<const float*>(value));
  }
}
void GLWidget::cbGetSpec(void * value, void * clientData)
{
  auto sponza_model = *reinterpret_cast<std::shared_ptr<fly::Model>*>(clientData);
  *reinterpret_cast<float*>(value) = sponza_model->getMaterials().front()->getSpecularExponent();
}
void GLWidget::cbSetShadows(const void * value, void * clientData)
{
  fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
  settings._shadows = *reinterpret_cast<const bool*>(value);
  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
}
void GLWidget::cbGetShadows(void * value, void * clientData)
{
  *reinterpret_cast<bool*>(value) = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings()._shadows;
}
void GLWidget::cbReloadShaders(void * client_data)
{
  (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(client_data))->reloadShaders();
}
void GLWidget::cbSetSortModeShaderMaterial(const void * value, void * clientData)
{
  if (*reinterpret_cast<const bool*>(value)) {
    fly::Settings settings = (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->getSettings();
    settings._dlSortMode = fly::DisplayListSortMode::SHADER_AND_MATERIAL;
    (*reinterpret_cast<std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>>*>(clientData))->setSettings(settings);
  }
}

void GLWidget::initGame()
{
  auto importer = std::make_shared<fly::AssimpImporter>();
  _sponzaModel = importer->loadModel("assets/sponza/sponza.obj");
  for (const auto& m : _sponzaModel->getMaterials()) {
    m->setSpecularExponent(32.f);
  }
#if SPONZA_MANY
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
#endif
      for (const auto& mesh : _sponzaModel->getMeshes()) {
        auto entity = _engine->getEntityManager()->createEntity();
        entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(mesh,
#if SPONZA_MANY
          _sponzaModel->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(x * 60.f, 0.f, y * 60.f), fly::Vec3f(0.01f)).getModelMatrix()));
#else
          _sponzaModel->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(0.f), fly::Vec3f(0.01f)).getModelMatrix()));
#endif
      }
#if SPONZA_MANY
    }
  }
#endif

#if SPONZA_MANY
  auto plane_model = importer->loadModel("assets/plane.obj");
  for (const auto& m : plane_model->getMaterials()) {
    m->setNormalPath("assets/ground_normals.png");
    m->setDiffuseColor(fly::Vec3f(0.870f, 0.768f, 0.329f) * 1.5f);
  }
  for (const auto& m : plane_model->getMeshes()) {
    std::vector<fly::Vertex> vertices_new;
    for (const auto& v : m->getVertices()) {
      fly::Vertex v_new = v;
      v_new._uv *= 120.f;
      vertices_new.push_back(v_new);
    }
    m->setVertices(vertices_new);
  }
  for (const auto& m : plane_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    auto scale = _renderer->getSceneMax() - _renderer->getSceneMin();
    scale[1] = 1.f;
    auto translation = _renderer->getSceneMin();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m, 
      plane_model->getMaterials()[m->getMaterialIndex()], fly::Transform(translation, scale).getModelMatrix()));
  }
#endif

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f)));
  
  auto dl_entity = _engine->getEntityManager()->createEntity();
  std::vector<float> csm_distances = { 30.f, 50.f, 200.f };
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(-500.f, 0.f, -500.f), csm_distances);
 // auto light_pos = _renderer->getSceneMin();
  //light_pos[1] = 200.f;
  //_dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), light_pos, glm::vec3(0.f), csm_distances);
  dl_entity->addComponent(_dl);
  
  _camController = std::make_unique<fly::CameraController>(cam_entity->getComponent<fly::Camera>(), 20.f);
}
