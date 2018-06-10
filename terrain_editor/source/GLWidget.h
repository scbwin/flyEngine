#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <GL/glew.h>
#include <QtOpenGL>
#include <renderer/ProjectionParams.h>
#include <math/FlyMath.h>
#include <Camera.h>
#include <Engine.h>
#include <GraphicsSettings.h>
#include <set>

namespace fly
{
  class OpenGLAPI;
  template<typename API, typename BV>
  class Renderer;
  template<typename API, typename BV>
  class SkydomeRenderable;
  class GameTimer;
  class CameraController;
  class PhysicsCameraController;
  class DirectionalLight;
  class Model;
  class Bullet3PhysicsSystem;
  class Entity;
  class RigidBody;
  class Camera;
  class AABB;
  class Sphere;
}

class GLWidget : public QOpenGLWidget
{
public:
  GLWidget();
  ~GLWidget();
protected:
  virtual void initializeGL() override;
  virtual void resizeGL(int width, int height) override;
  virtual void paintGL() override;
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
  std::set<int> _keysPressed;
  template<typename T>
  inline bool contains(const std::set<T>& set, const T& t)
  {
    return set.find(t) != set.end();
  }

  fly::ProjectionParams _pp;
  fly::Mat4f _projectionMatrix;
  std::shared_ptr<fly::Camera> _camera;
  fly::Engine _engine;
  std::shared_ptr<fly::Renderer<fly::OpenGLAPI, fly::AABB>> _renderer;
  std::shared_ptr<fly::GraphicsSettings> _gs;
  std::shared_ptr<fly::DirectionalLight> _dl;
  std::shared_ptr<fly::PhysicsCameraController> _physicsCC;
  std::shared_ptr<fly::CameraController> _cc;
  float _camAccelerationDefault = 200.f;
  float _camAccelerationHigh = 400.f;
  float _camAccelerationLow = 100.f;
};

#endif // !GLWIDGET_H
