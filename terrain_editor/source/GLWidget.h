#ifndef GLWIDGET_H
#define GLWIDGET_H
#include <opengl/GLShaderInterface.h>
#include <opengl/GLBuffer.h>
#include <GL/glew.h>
#include <QtOpenGL>
#include <renderer/ProjectionParams.h>
#include <math/FlyMath.h>
#include <Camera.h>
#include <Engine.h>
#include <GraphicsSettings.h>
#include <set>
#include <opengl/GLShaderProgram.h>
#include <opengl/GLVertexArray.h>
#include <AntTweakBar.h>
#include <opengl/GLTexture.h>
#include <opengl/GLVertexArray.h>

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

class GLWidget : public QOpenGLWidget, public QOpenGLExtraFunctions
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
 // std::shared_ptr<fly::Renderer<fly::OpenGLAPI, fly::AABB>> _renderer;
  std::shared_ptr<fly::GraphicsSettings> _gs;
  std::shared_ptr<fly::DirectionalLight> _dl;
  std::shared_ptr<fly::PhysicsCameraController> _physicsCC;
  std::shared_ptr<fly::CameraController> _cc;
  float _camAccelerationDefault = 20.f;
  float _camAccelerationHigh = 40.f;
  float _camAccelerationLow = 10.f;
  std::unique_ptr<fly::GLShaderProgram> _terrainShader;
  std::unique_ptr<fly::GLShaderProgram> _terrainShaderWireframe;
  int _patchesPerDir = 16 * 32;
  fly::Vec3f _seed = fly::Vec3f(24.33f, 52.23f, 14721.28f);
  float _amplitude = 10.f;
  float _frequency = 0.02f;
  float _amplitudeSky = 8.f;
  float _frequencySky = 10.f;
  float _exponentSky = 1.556f;
  float _persistenceSky = 0.2195f;
  float _lacunaritySky = 3.0063f;
  int _patchSize = 6;
  unsigned _octaves = 8;
  float _maxTessDistance = 512.f;
#if 1
    float _exponent = 1.556f;
    float _persistence = 0.2195f;
   float _lacunarity = 3.0063f;
    bool _terraces = false;
    fly::Vec3f _terrainColor = 0.3f;
#else
  float _exponent = 1.25f;
  float _persistence = 0.35f;
  float _lacunarity = 2.6f;
  bool _terraces = true;
  fly::Vec3f _terrainColor = fly::Vec3f(0.486f, 0.305f, 0.094f);
#endif
  fly::Vec2f _snowLevel = fly::Vec2f(15.f, 50.f);
  fly::Vec3f _groundColor = fly::Vec3f(44.f, 93.f, 0.f) / 255.f;
  fly::Vec3f _groundColor2 = fly::Vec3f(255.f, 255.f, 118.f) / 255.f;
  fly::Vec2f _groundLevel = fly::Vec2f(3.f, 20.f);
  float _snowIntensity = 10.f;
  float _steepnessExponent = 6.f;
  float _steepnessExponentLow = 2.f;
  float _ambient = 0.f;
  float _fogDistance = 500.f;
  fly::Vec3f _fogColor = fly::Vec3f(1.f);
  std::unique_ptr<fly::GLVertexArray> _skydomeVao;
  std::unique_ptr<fly::GLBuffer> _skydomeVbo;
  std::unique_ptr<fly::GLBuffer> _skydomeIbo;
  std::unique_ptr<fly::GLShaderProgram> _skydomeShader;
  unsigned _skydomeIndices;
  bool _wireFrame = false;

  static void setLightDirX(const void* value, void* client_data);
  static void getLightDirX(void* value, void* client_data);
  static void setLightDirY(const void* value, void* client_data);
  static void getLightDirY(void* value, void* client_data);
  static void setLightDirZ(const void* value, void* client_data);
  static void getLightDirZ(void* value, void* client_data);

};

#endif // !GLWIDGET_H
