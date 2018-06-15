#ifndef BASEQTSCENE_H
#define BASEQTSCENE_H

#include <opengl/GLShaderInterface.h>
#include <opengl/OpenGLAPI.h>
#include <QtOpenGL>
#include <set>
#include <memory>
#include <Engine.h>
#include <math/FlyMath.h>
#include <GraphicsSettings.h>

namespace fly
{
  class DirectionalLight;
  class Camera;
  class CameraController;
  class PhysicsCameraController;
  template<typename API, typename BV>
  class Renderer;
}

class BaseQtScene : public QOpenGLWidget
{
  public:
    BaseQtScene();
    virtual ~BaseQtScene();
  protected:
    virtual void keyPressEvent(QKeyEvent* e) override;
    virtual void keyReleaseEvent(QKeyEvent* e) override;
    virtual void mousePressEvent(QMouseEvent* e) override;
    virtual void mouseMoveEvent(QMouseEvent* e) override;
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
    virtual void wheelEvent(QWheelEvent* e) override;
    virtual void initializeGL() override;
    virtual void resizeGL(int width, int height) override;
    virtual void paintGL() override;
    virtual void initScene() = 0;
    std::set<int> _keysPressed;
    template<typename T>
    inline bool contains(const std::set<T>& set, const T& t)
    {
      return set.find(t) != set.end();
    }
    std::shared_ptr<fly::DirectionalLight> _dl;
    std::shared_ptr<fly::Camera> _camera;
    std::unique_ptr<fly::CameraController> _camController;
    std::shared_ptr<fly::PhysicsCameraController> _physicsCC;
    float _camAccelerationDefault = 200.f;
    float _camAccelerationHigh = 400.f;
    float _camAccelerationLow = 100.f;
    fly::Engine _engine;
    std::shared_ptr<fly::Renderer<fly::OpenGLAPI, fly::AABB>> _renderer;
    fly::GraphicsSettings _gs;
private:
  void handleInput();
};

#endif