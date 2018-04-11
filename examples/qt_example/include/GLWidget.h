#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QtOpenGL>
#include <memory>
#include <set>

#define SPONZA_MANY 0

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
  static void cbSetLightIntensity(const void* value, void* clientData);
  static void cbGetLightIntensity(void* value, void* clientData);
  static void cbSetPostProcessing(const void* value, void* clientData);
  static void cbGetPostProcessing(void* value, void* clientData);
  static void cbSetSpec(const void* value, void* clientData);
  static void cbGetSpec(void* value, void* clientData);
  static void cbSetShadows(const void* value, void* clientData);
  static void cbGetShadows(void* value, void* clientData);
  static void cbReloadShaders(void* client_data);
  static void cbSetPCF(const void* value, void* client_data);
  static void cbGetPCF(void* value, void* client_data);
  static void cbSetSmBias(const void* value, void* client_data);
  static void cbGetSmBias(void* value, void* client_data);
  static void cbSetNormalMapping(const void* value, void* client_data);
  static void cbGetNormalMapping(void* value, void* client_data);
  static void cbSetParallaxMapping(const void* value, void* client_data);
  static void cbGetParallaxMapping(void* value, void* client_data);
  static void cbSetParallaxSteep(const void* value, void* client_data);
  static void cbGetParallaxSteep(void* value, void* client_data);
  static void cbSetPMHScale(const void* value, void* client_data);
  static void cbGetPMHScale(void* value, void* client_data);
  static void cbSetPMMinSteps(const void* value, void* client_data);
  static void cbGetPMMinSteps(void* value, void* client_data);
  static void cbSetPMMaxSteps(const void* value, void* client_data);
  static void cbGetPMMaxSteps(void* value, void* client_data);
private:
  std::unique_ptr<fly::Engine> _engine;
  std::shared_ptr<fly::AbstractRenderer<fly::OpenGLAPI>> _renderer;
  std::unique_ptr<fly::GameTimer> _gameTimer;
  std::unique_ptr<fly::CameraController> _camController;
  std::shared_ptr<fly::DirectionalLight> _dl;
  void initGame();
  float _measure = 0.f;
  unsigned _fps = 0;
  CTwBar* _bar;
  const char* _fpsButtonName = "FPS";
};

#endif