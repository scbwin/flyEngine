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

GLWidget::GLWidget()
{
  _engine = std::unique_ptr<fly::Engine>(new fly::Engine());
  _gameTimer = std::unique_ptr<fly::GameTimer>(new fly::GameTimer());
}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
  _renderer = std::make_shared<fly::AbstractRenderer<fly::OpenGLAPI>>();
  _engine->addSystem(_renderer);
  initGame();
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2u({ width, height }));
}

void GLWidget::paintGL()
{
  _gameTimer->tick();
  _engine->update(_gameTimer->getTimeSeconds(), _gameTimer->getDeltaTimeSeconds());
  if (keyPressed('W')) {
    _camController->stepForward(_gameTimer->getDeltaTimeSeconds());
  }
  if (keyPressed('A')) {
    _camController->stepLeft(_gameTimer->getDeltaTimeSeconds());
  }
  if (keyPressed('S')) {
    _camController->stepBackward(_gameTimer->getDeltaTimeSeconds());
  }
  if (keyPressed('D')) {
    _camController->stepRight(_gameTimer->getDeltaTimeSeconds());
  }
  if (keyPressed('C')) {
    _camController->stepDown(_gameTimer->getDeltaTimeSeconds());
  }
  if (keyPressed(Qt::Key::Key_Space)) {
    _camController->stepUp(_gameTimer->getDeltaTimeSeconds());
  }
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
    _camController->mousePress(fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  _camController->mouseMove(_gameTimer->getDeltaTimeSeconds(), fly::Vec3f({ static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f }));
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    _camController->mouseRelease();
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}

void GLWidget::initGame()
{
  auto importer = std::make_shared<fly::AssimpImporter>();
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  auto sponza_entity = _engine->getEntityManager()->createEntity();
  sponza_entity->addComponent(std::make_shared<fly::Transform>(fly::Vec3f(0.f), fly::Vec3f(0.01f)));
  std::vector<std::shared_ptr<fly::Model>> lods = { sponza_model };
  sponza_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(lods, sponza_entity->getComponent<fly::Transform>(), 60.f));

  std::vector<std::shared_ptr<fly::Model>> sphere_models = { importer->loadModel("assets/sphere_lod0.obj"),
    importer->loadModel("assets/sphere_lod1.obj"), importer->loadModel("assets/sphere_lod2.obj") };
  auto sphere_entity = _engine->getEntityManager()->createEntity();
  sphere_entity->addComponent(std::make_shared<fly::StaticModelRenderable>(sphere_models, std::make_shared<fly::Transform>(glm::vec3(50.f, 0.f, 0.f)), 5.f));

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 0.f, -100.f), glm::vec3(0.f)));
  
  _camController = std::unique_ptr<fly::CameraController>(new fly::CameraController(cam_entity->getComponent<fly::Camera>(), 20.f));
}
