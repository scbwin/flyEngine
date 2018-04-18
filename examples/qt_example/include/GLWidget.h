#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGL>
#include <memory>
#include <set>
#include <GraphicsSettings.h>

#define SPONZA_MANY 1
#define TREE_SCENE 0
#define PHYSICS 0
#define SKYDOME 1
#define NUM_OBJECTS 100

class btTriangleMesh;

namespace fly
{
  class OpenGLAPI;
  template<class T>
  class AbstractRenderer;
  class Engine;
  class GameTimer;
  class CameraController;
  class DirectionalLight;
  class Model;
  class Bullet3PhysicsSystem;
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
  std::unique_ptr<fly::Engine> _engine;
  fly::GraphicsSettings _graphicsSettings;
  std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>> _renderer;
  std::shared_ptr<fly::Bullet3PhysicsSystem> _physicsSystem;
  std::unique_ptr<fly::GameTimer> _gameTimer;
  std::unique_ptr<fly::CameraController> _camController;
  std::shared_ptr<fly::DirectionalLight> _dl;
  std::vector<std::shared_ptr<btTriangleMesh>> _triangleMeshes;
  void initGame();
  float _measure = 0.f;
  unsigned _fps = 0;
  CTwBar* _bar;
  const char* _fpsButtonName = "FPS";
  const char* _renderedTrianglesName = "Triangles";
  const char* _renderedTrianglesShadowName = "Triangles shadow";
  const char* _renderedMeshesName = "Meshes";
  const char* _renderedMeshesShadowName = "Meshes shadow";
  const char* _bvhTraversalName = "BVH traversal microseconds";
  const char* _sceneRenderingCPUName = "CPU scene rendering time";
  std::string formatNumber(unsigned number);
};

#endif