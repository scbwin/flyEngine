#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <AssimpImporter.h>
#include <btBulletDynamicsCommon.h>
#include <QtOpenGL>
#include <Engine.h>
#include <physics/RigidBody.h>
#include <physics/Bullet3PhysicsSystem.h>
#include <memory>
#include <set>
#include <GraphicsSettings.h>
#include <math/FlyMath.h>

#define SPONZA 1
#define SPONZA_MANY 0
#define TOWERS 0
#define TREE_SCENE 0
#define PHYSICS 0
#define SKYDOME 1
#define NUM_OBJECTS 100
#define NUM_TOWERS 15
#define DELETE_CURTAIN 1
#define INSTANCED_MESHES 0
//#define NUM_INSTANCED_MESHES_PER_DIR 1000
#define NUM_CELLS 64
#define ITEMS_PER_CELL 64

class btTriangleMesh;

namespace fly
{
  class OpenGLAPI;
  template<class T>
  class Renderer;
  class GameTimer;
  class CameraController;
  class DirectionalLight;
  class Model;
  class Bullet3PhysicsSystem;
  class Entity;
  class RigidBody;
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
  std::shared_ptr<fly::Renderer<fly::OpenGLAPI>> _renderer;
  std::unique_ptr<fly::CameraController> _camController;
  std::shared_ptr<fly::DirectionalLight> _dl;
  std::shared_ptr<fly::Entity> _skydome;
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

  std::string formatNumber(unsigned number);
};

#endif