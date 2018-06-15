#include <BaseQtScene.h>
#include <Light.h>
#include <Camera.h>
#include <PhysicsCameraController.h>
#include <CameraController.h>
#include <renderer/Renderer.h>

BaseQtScene::BaseQtScene() : QOpenGLWidget(nullptr),
  _dl(std::make_shared<fly::DirectionalLight>(fly::Vec3f(1.f), fly::Vec3f(0.5f, -1.f, 0.5f ))),
  _camera(std::make_shared<fly::Camera>(fly::Vec3f(0.f, 0.f, -10.f), fly::Vec3f(0.f, 0.f, 0.f))),
  _camController(std::make_unique<fly::CameraController>(_camera, 20.f)),
  _physicsCC(std::make_unique<fly::PhysicsCameraController>(_camera))
{
}

BaseQtScene::~BaseQtScene()
{
}

void BaseQtScene::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
}

void BaseQtScene::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());
}

void BaseQtScene::mousePressEvent(QMouseEvent * e)
{
}

void BaseQtScene::mouseMoveEvent(QMouseEvent * e)
{
}

void BaseQtScene::mouseReleaseEvent(QMouseEvent * e)
{
}

void BaseQtScene::wheelEvent(QWheelEvent * e)
{
}

void BaseQtScene::initializeGL()
{
  _renderer = std::make_shared<fly::Renderer<fly::OpenGLAPI, fly::AABB>>(&_gs);
  _gs.addListener(_renderer);

  initScene();

  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(BaseQtScene::*)()>(&BaseQtScene::update));
  timer->start(0);

  _renderer->setCamera(_camera);
  _renderer->setDirectionalLight(_dl);
  _renderer->buildBVH();
  _engine.addSystem(_renderer);
  _engine.addSystem(_physicsCC);
}

void BaseQtScene::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i(width, height));
}

void BaseQtScene::paintGL()
{
  handleInput();
  _renderer->setDefaultRendertarget(defaultFramebufferObject());
  _engine.update();
}

void BaseQtScene::handleInput()
{
  fly::Vec3f acc_dir(0.f);
  if (contains<int>(_keysPressed, 'W')) {
    acc_dir += _camera->getDirection();
  }
  if (contains<int>(_keysPressed, 'A')) {
    acc_dir -= _camera->getRight();
  }
  if (contains<int>(_keysPressed, 'S')) {
    acc_dir -= _camera->getDirection();
  }
  if (contains<int>(_keysPressed, 'D')) {
    acc_dir += _camera->getRight();
  }
  if (contains<int>(_keysPressed, 'C')) {
    acc_dir -= _camera->getUp();
  }
  if (contains<int>(_keysPressed, Qt::Key::Key_Space)) {
    acc_dir += _camera->getUp();
  }
  float acc;
  if (contains<int>(_keysPressed, Qt::Key::Key_Shift)) {
    acc = _camAccelerationHigh;
  }
  else {
    acc = contains<int>(_keysPressed, Qt::Key::Key_Control) ? _camAccelerationLow : _camAccelerationDefault;
  }
  _physicsCC->setAcceleration(acc_dir, acc);
}
