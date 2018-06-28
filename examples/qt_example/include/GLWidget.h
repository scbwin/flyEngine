#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <AssimpImporter.h>
//#include <btBulletDynamicsCommon.h>
#include <QtOpenGL>
#include <Engine.h>
//#include <physics/RigidBody.h>
//#include <physics/Bullet3PhysicsSystem.h>
#include <memory>
#include <set>
#include <GraphicsSettings.h>
#include <math/FlyMath.h>
#include <AntWrapper.h>

#define SPONZA 1
#define SPONZA_MANY 0 && SPONZA
#define TOWERS 0
#define TREE_SCENE 0
#define PHYSICS 0
#define SKYDOME 1
#define NUM_OBJECTS 100
#define NUM_TOWERS 15
#define DELETE_CURTAIN 1
#define INSTANCED_MESHES 1 && !SPONZA
#define NUM_CELLS 64
#define ITEMS_PER_CELL NUM_CELLS
#define SINGLE_SPHERE 0
#define TINY_MESHES_PER_DIR 128
#define TINY_RENDERER_MODELS 1 && !SPONZA
#define TINY_RENDERER_INSTANCED 0 && TINY_RENDERER_MODELS

class btTriangleMesh;

namespace fly
{
  class OpenGLAPI;
  template<typename API, typename BV>
  class Renderer;
  template<typename API, typename BV>
  class SkydomeRenderable;
  template<typename API, typename BV>
  class CamSpeedSystem;
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

struct CTwBar;

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
private:
  fly::Engine _engine;
  fly::GraphicsSettings _graphicsSettings;
  std::shared_ptr<fly::Renderer<fly::OpenGLAPI, fly::AABB>> _renderer;
  std::unique_ptr<fly::CameraController> _camController;
  std::shared_ptr<fly::PhysicsCameraController> _physicsCC;
  std::shared_ptr<fly::DirectionalLight> _dl;
  std::shared_ptr<fly::SkydomeRenderable<fly::OpenGLAPI, fly::AABB>> _skydome;
  std::shared_ptr<fly::Camera> _camera;
  std::shared_ptr<fly::Camera> _debugCamera;
  std::shared_ptr<fly::CamSpeedSystem<fly::OpenGLAPI, fly::AABB>> _camSpeedSytem;
  std::unique_ptr<AntWrapper> _antWrapper;
  float _camAccelerationDefault = 200.f;
  float _camAccelerationHigh = 400.f;
  float _camAccelerationLow = 100.f;
  void updateStats();
#if PHYSICS
  std::shared_ptr<fly::Bullet3PhysicsSystem> _physicsSystem;
  std::vector<std::shared_ptr<btTriangleMesh>> _triangleMeshes;
  std::vector<std::shared_ptr<fly::RigidBody>> _rigidBodys;
  fly::RigidBody* _selectedRigidBody = nullptr;
  fly::Vec2f _viewPortSize;
  float _focusDist = 3.f;
  float _impulseStrength = 0.001f;
  float _impulseStrengthSmash = 0.65f;
  bool _smashItem = false;
#endif
  void initGame();
  float _measure = 0.f;
  unsigned _fps = 0;
  CTwBar* _bar;
  const char* _fpsButtonName = "FPS";
  const char* _rendererCPUTimeName = "Renderer CPU time";
  const char* _renderedTrianglesName = "Triangles";
  const char* _renderedTrianglesShadowName = "Triangles shadow";
  const char* _renderedMeshesName = "Meshes";
  const char* _renderedMeshesShadowName = "Meshes shadow";
  const char* _cullingName = "Culling microseconds";
  const char* _cullingShadowMapName = "Culling shadow map microseconds";
  const char* _sceneRenderingCPUName = "CPU scene rendering time";
  const char* _smRenderingCPUName = "CPU shadow map rendering time";
  const char* _sceneMeshGroupingUName = "Scene mesh grouping time";
  const char* _shadowMapGroupingUName = "Shadow map grouping time";
  const char* _rendererIdleTimeName = "Renderer idle time";

  std::string formatNumber(unsigned number);
};

#endif