#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGL>
#include <memory>
#include <set>

namespace fly
{
  class OpenGLAPI;
  template<class T>
  class AbstractRenderer;
  class Engine;
  class GameTimer;
  class CameraController;
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
  //std::set<int> _buttonsPressed;
  template<typename T>
  inline bool contains(const std::set<T>& set, const T& t) 
  {
    return set.find(t) != set.end();
  }
  static void cbSetDebugQuadtreeAABBs(const void *value, void *clientData);
  static void cbGetDebugQuadtreeAABBs(void *value, void *clientData);
  static void cbSetDebugObjectAABBs(const void *value, void *clientData);
  static void cbGetDebugObjectAABBs(void *value, void *clientData);
  static void cbSetSortModeMaterial(const void *value, void *clientData);
  static void cbGetSortModeMaterial(void *value, void* clientData);
  static void cbSetSortModeShaderMaterial(const void *value, void *clientData);
  static void cbGetSortModeShaderMaterial(void *value, void* clientData);
private:
  std::unique_ptr<fly::Engine> _engine;
  std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>> _renderer;
  std::unique_ptr<fly::GameTimer> _gameTimer;
  std::unique_ptr<fly::CameraController> _camController;
  void initGame();
  float _measure = 0.f;
  unsigned _fps = 0;
  CTwBar* _bar;
  const char* _fpsButtonName = "FPS";
};

#endif