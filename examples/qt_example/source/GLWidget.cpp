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
#include <AntWrapper.h>

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
  _renderer = std::make_shared<fly::AbstractRenderer<fly::OpenGLAPI>>(&_graphicsSettings);
  _graphicsSettings.addListener(_renderer);
  _engine->addSystem(_renderer);
  initGame();
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
  TwInit(TwGraphAPI::TW_OPENGL_CORE, nullptr);
  _bar = TwNewBar("Stats");
  TwAddButton(_bar, _fpsButtonName, nullptr, nullptr, nullptr);
  auto settings_bar = TwNewBar("Settings");
  AntWrapper(settings_bar, &_graphicsSettings, _renderer->getApi());
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

void GLWidget::initGame()
{
  auto importer = std::make_shared<fly::AssimpImporter>();
#if TREE_SCENE
  auto tree_model = importer->loadModel("assets/Tree1/Tree1.obj");
  for (const auto& m : tree_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m, tree_model->getMaterials()[m->getMaterialIndex()], fly::Transform().getModelMatrix(), false));
  }
#else
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  for (const auto& m : sponza_model->getMaterials()) {
    m->setSpecularExponent(32.f);
    if (m->getDiffusePath() == "assets/sponza/textures\\spnza_bricks_a_diff.tga") {
      m->setHeightPath("assets/DisplacementMap.png");
    }
    else if (m->getDiffusePath() == "assets/sponza/textures\\sponza_floor_a_diff.tga") {
      m->setHeightPath("assets/height.png");
    }
  }

#if SPONZA_MANY
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
#endif
      unsigned index = 0;
      for (const auto& mesh : sponza_model->getMeshes()) {
        auto entity = _engine->getEntityManager()->createEntity();
        bool has_wind = index >= 44 && index <= 62;
        fly::Vec3f aabb_offset = has_wind ? fly::Vec3f(0.f, 0.f, 0.25f) : fly::Vec3f(0.f);
        if (index == sponza_model->getMeshes().size() - 28) {
          has_wind = true;
          aabb_offset = fly::Vec3f(0.f, 0.f, 0.25f);
        }
        entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(mesh,
#if SPONZA_MANY
          sponza_model->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(x * 60.f, 0.f, y * 60.f), fly::Vec3f(0.01f)).getModelMatrix(), has_wind, aabb_offset));
#else
          sponza_model->getMaterials()[mesh->getMaterialIndex()], fly::Transform(fly::Vec3f(0.f), fly::Vec3f(0.01f)).getModelMatrix(), has_wind, aabb_offset));
#endif
        index++;
      }
#if SPONZA_MANY
    }
  }
#endif
#endif

#if SPONZA_MANY || TREE_SCENE
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
      plane_model->getMaterials()[m->getMaterialIndex()], fly::Transform(translation, scale).getModelMatrix(), false));
  }
#endif

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f)));
  auto dl_entity = _engine->getEntityManager()->createEntity();
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(-500.f, 0.f, -500.f));
 // auto light_pos = _renderer->getSceneMin();
  //light_pos[1] = 200.f;
  //_dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), light_pos, glm::vec3(0.f), csm_distances);
  dl_entity->addComponent(_dl);
  
  _camController = std::make_unique<fly::CameraController>(cam_entity->getComponent<fly::Camera>(), 20.f);
#if SPONZA_MANY
  _camController->setSpeed(100.f);
#endif
}
