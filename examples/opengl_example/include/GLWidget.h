#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <opencv2/opencv.hpp>
#include "opengl/RenderingSystemOpenGL.h"
#include <qopenglwidget.h>
#include <QtOpenGL/qgl.h>
#include <memory>
#include "Engine.h"
#include <set>
#include <qlabel.h>
#include <qwidget.h>
#include <chrono>
#include <qcolordialog.h>
#include <Camera.h>
#include <Model.h>
#include <random>
#include <GameTimer.h>
#include <AssimpImporter.h>

#define SPONZA 1

class GLWidget : public QOpenGLWidget
{
  Q_OBJECT
public:
  GLWidget();
protected:
  virtual void initializeGL() override;
  virtual void paintGL() override;
  virtual void resizeGL(int width, int height) override;
  virtual void keyPressEvent(QKeyEvent* e) override;
  virtual void keyReleaseEvent(QKeyEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void wheelEvent(QWheelEvent* e) override;
private:
  void initGui();
  void showGui();
  void hideGui();
  std::shared_ptr<fly::Engine> _engine;
  std::shared_ptr<fly::RenderingSystemOpenGL> _rs;
  std::vector<std::shared_ptr<fly::Entity>> _cameras;
  std::vector<std::shared_ptr<fly::Entity>> _lights;
  bool _guiWasHidden = false;
  const std::chrono::high_resolution_clock::time_point _timePointStart;
  std::chrono::high_resolution_clock::time_point _timePoint;
  fly::GameTimer _gameTimer;
  QLabel* _fpsLabel;
  QFont _font;
  QFont _fontBig;
  QWidget* _guiLeft;
  QWidget* _guiWidget;
  QColorDialog* _colorDialog;
  std::unique_ptr<fly::IImporter> _importer = std::make_unique<fly::AssimpImporter>();

#if PROFILE
  QLabel* _lblTreeDrawCalls;
  QLabel* _lblTreesMs;
  QLabel* _lblTreesShadowMapMs;
  QLabel* _lblTreeTriangles;
  QLabel* _lblTreeShadowMapTriangles;

  QLabel* _lblGrassDrawCalls;
  QLabel* _lblGrassMs;
  QLabel* _lblGrassBlades;
  QLabel* _lblGrassTriangles;
#endif
  QColor _guiBackgroundColor = QColor(50, 50, 50, 128);
  std::vector<float> _csmDistances = { 10.f, 30.f, 125.f, 300.f };
  //std::shared_ptr<fly::Mesh> _sphereMesh;
  std::shared_ptr<fly::Model> _sphereModel;
#if !SPONZA
  std::shared_ptr<fly::Entity> _geoMipMapEntity;
#endif
  unsigned _frameCounter = 0;
  double _deltaTime = 1000.0 / 60.0;
  double _time;
  float _lightDistWhenClicked = 0.f;
  glm::vec3 _lightTargetWorld = glm::vec3(0.f);
  glm::vec2 _mouseDelta;
  std::set<int> _keysPressed;
  std::set<Qt::MouseButton> _buttonsPressed;
  void handleKeyEvents();
  void updateLabels();
  float bernstein(float u, int i, int n);
  int binomial(int i, int n);
  int factorial(int n);
  glm::vec2 deCasteljau(float u, const std::vector<glm::vec2>& points, glm::vec2& tangent);
  cv::Mat generateSplatMap(int size);
  void generateSplatMap(const glm::vec2& p1, const glm::vec2& p2, std::mt19937& gen, std::uniform_real_distribution<float>& dist, cv::Mat& splat_map);
  enum class Mode
  {
    LIGHT, CAMERA, LIGHT_MOVE, EDIT_TERRAIN
  };
  Mode _mode = Mode::CAMERA;
};

#endif // ! GLWIDGET_H
