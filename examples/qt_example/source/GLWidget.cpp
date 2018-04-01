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
  _engine = std::unique_ptr<fly::Engine>(new fly::Engine());
  _gameTimer = std::unique_ptr<fly::GameTimer>(new fly::GameTimer());
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

  auto settings_bar = TwNewBar("Settings");
  TwAddVarCB(settings_bar, "Group by material", TwType::TW_TYPE_BOOLCPP, cbSetSortModeMaterial, cbGetSortModeMaterial, &_renderer, nullptr);
  TwAddVarCB(settings_bar, "Group by shader and material", TwType::TW_TYPE_BOOLCPP, cbSetSortModeShaderMaterial, cbGetSortModeShaderMaterial, &_renderer, nullptr);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i( width, height ));
  TwWindowSize(width, height);
}

void GLWidget::paintGL()
{
  _gameTimer->tick();
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
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  for (const auto& mesh : sponza_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(mesh, 
      sponza_model->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(0.f), fly::Vec3f(0.01f)).getModelMatrix()));
  }

  /*std::vector<std::shared_ptr<fly::Model>> sphere_models = { importer->loadModel("assets/sphere_lod0.obj"),
    importer->loadModel("assets/sphere_lod1.obj"), importer->loadModel("assets/sphere_lod2.obj") };
  auto sphere_entity = _engine->getEntityManager()->createEntity();
  sphere_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(sphere_models, std::make_shared<fly::Transform>(glm::vec3(50.f, 0.f, 0.f)), 5.f));*/

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f)));
  
  auto dl_entity = _engine->getEntityManager()->createEntity();
  std::vector<float> csm_distances = { 10.f, 50.f, 200.f };
  dl_entity->addComponent(std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(30.f, 50.f, 0.f), glm::vec3(0.f), csm_distances));
  
  _camController = std::unique_ptr<fly::CameraController>(new fly::CameraController(cam_entity->getComponent<fly::Camera>(), 20.f));
}
