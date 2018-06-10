#include <opengl/OpenGLAPI.h>
#include <AssimpImporter.h>
#include <GLWidget.h>
#include <iostream>
#include <opengl/OpenGLUtils.h>
#include <math/MathHelpers.h>
#include <renderer/Renderer.h>
#include <Model.h>
#include <Mesh.h>
#include <Material.h>
#include <renderer/MeshRenderables.h>
#include <PhysicsCameraController.h>
#include <CameraController.h>

GLWidget::GLWidget() :
  _camera(std::make_shared<fly::Camera>(fly::Vec3f(0.f), fly::Vec3f(0.f, 0.f, 0.f))),
  _dl(std::make_shared<fly::DirectionalLight>(fly::Vec3f(1.f), fly::Vec3f(0.5f, -1.f, 0.5f))),
  _physicsCC(std::make_shared<fly::PhysicsCameraController>(_camera)),
  _cc(std::make_shared<fly::CameraController>(_camera, 20.f))
{
  _pp._near = 0.1f;
  _pp._far = 1000.f;
  _pp._fieldOfViewDegrees = 45.f;
}

GLWidget::~GLWidget()
{
}

void GLWidget::initializeGL()
{
  _gs = std::make_shared<fly::GraphicsSettings>();
  _renderer = std::make_shared<fly::Renderer<fly::OpenGLAPI, fly::AABB>>(_gs.get());
  _renderer->setCamera(_camera);
  _renderer->setDirectionalLight(_dl);
  _gs->addListener(_renderer);
  _engine.addSystem(_renderer);
  _engine.addSystem(_physicsCC);

  auto model = fly::AssimpImporter().loadModel("assets/cube.obj");
  auto mesh = model->getMeshes().front();
  _renderer->addStaticMeshRenderable(std::make_shared<fly::StaticMeshRenderable<fly::OpenGLAPI, fly::AABB>>(*_renderer, mesh, mesh->getMaterial(), fly::Transform(fly::Vec3f(0.f, 0.f, 10.f))));
  _renderer->addStaticMeshRenderable(std::make_shared<fly::StaticMeshRenderable<fly::OpenGLAPI, fly::AABB>>(*_renderer, mesh, mesh->getMaterial(), fly::Transform(fly::Vec3f(0.f, -3.f, 10.f))));
  _renderer->buildBVH();

  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i(width, height));
 // _projectionMatrix = fly::MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, static_cast<float>(width) / static_cast<float>(height), _pp._near, _pp._far, fly::ZNearMapping::MINUS_ONE);
}

void GLWidget::paintGL()
{
  _renderer->setDefaultRendertarget(defaultFramebufferObject());
  _engine.update();

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
  _cc->mousePress(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  _cc->mouseMove(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  _cc->mouseRelease();
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}
