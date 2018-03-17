#define GLM_ENABLE_EXPERIMENTAL
#include "GLWidget.h"
#include "EntityManager.h"
#include "Entity.h"
#include "Model.h"
#include <qtimer.h>
#include <qdatetime.h>
#include <iostream>
#include <qevent.h>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <qfontdatabase.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <ClickableLabel.h>
#include <QApplication>
#include <qcheckbox.h>
#include <qslider.h>
#include "Effects.h"
#include <qcolordialog.h>
#include <Animation.h>
#include <AnimationSystem.h>
#include <qradiobutton.h>
#include <Transform.h>
#include <Light.h>
#include <Billboard.h>
#include <Terrain.h>
#include <random>
#include <NoiseGen.h>

#define MANY_OBJECTS 0

GLWidget::GLWidget() : _timePointStart(std::chrono::high_resolution_clock::now())
{
  _engine = std::make_shared<fly::Engine>();
  _rs = std::make_shared<fly::RenderingSystemOpenGL>();
  _rs->setLightVolumesEnabled(false);
  _engine->addSystem(_rs);
  _engine->addSystem(std::make_shared<fly::AnimationSystem>());

  initGui();
}

void GLWidget::initializeGL()
{
  _rs->init(glm::ivec2(width(), height()));
  // _rs->setSkybox({ "assets/right.jpg", "assets/left.jpg", "assets/top.jpg", "assets/bottom.jpg", "assets/back.jpg", "assets/front.jpg" });
#if SPONZA
  auto sponza_model_instance = _importer->loadModel("assets/sponza/sponza.obj");
  sponza_model_instance->getMeshes().erase(sponza_model_instance->getMeshes().end() - 28);
   _rs->_zNear = 0.1f;
 #if MANY_OBJECTS

   auto plane_model = std::make_shared<fly::Model>("assets/plane.obj");
   for (auto& m : plane_model->getMeshes()) {
     for (auto& v : m->getVertices()) {
       v._uv *= 50.f;
     }
   }
   auto plane_entity = fly::EntityManager::instance().createEntity();
   plane_entity->addComponent(plane_model);
   float plane_scale = 400.f;
   plane_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(0.f, -1.2f, 0.f), glm::vec3(plane_scale, 1.f, plane_scale)));
   for (int x = -4; x <= 4; x++) {
     for (int z = -4; z <= 4; z++) {
 #endif
       auto sponza_model = std::make_shared<fly::Model>(*sponza_model_instance);
       auto sponza_entity = _engine->getEntityManager()->createEntity();
       sponza_entity->addComponent(sponza_model);
 #if MANY_OBJECTS
       sponza_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(x * 50, 0.f, z * 50), glm::vec3(0.01f)));

       auto billboard_entity = fly::EntityManager::instance().createEntity();
       billboard_entity->addComponent(std::make_shared<fly::Billboard>("assets/cloud.png", 0.5f));
       billboard_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(x * 500.f, 500.f, z * 500.f), glm::vec3(100.f)));
 #else
       sponza_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(0.f), glm::vec3(0.01f)));
 #endif
       for (auto& m : sponza_model->getMaterials()) {
         m.setSpecularExponent(64.f);
       }
 #if MANY_OBJECTS
     }
   }
 #endif

#else
  auto geo_mip_map = std::make_shared<fly::Terrain>(256);
  _geoMipMapEntity = _engine->getEntityManager()->createEntity();

  int grid_size = 256;
  int image_size = 4096;
//  auto splat_map = generateSplatMap(image_size);
  int num_cells = image_size / grid_size;
  auto noise_gen = fly::NoiseGen(grid_size);
  cv::Mat height_map(image_size, image_size, CV_32FC1, cv::Scalar(0));

  auto start = std::chrono::high_resolution_clock::now();
  float gain = 0.4f;
  unsigned int octaves = 8;
  for (int x = 0; x < image_size; x++) {
    for (int y = 0; y < image_size; y++) {
      glm::vec2 coord = glm::vec2(x, y) / static_cast<float>(num_cells);
      float amplitude = 1.f;
      float frequency = 0.03f;
      for (unsigned int i = 0; i < octaves; i++, amplitude *= gain, frequency *= 2.f) {
        height_map.at<float>(y, x) += amplitude * (1.f - abs(noise_gen.getPerlin(coord * frequency)));
      }
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Noise generation took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

  cv::Point min_loc, max_loc;
  double min, max;
  cv::minMaxLoc(height_map, &min, &max, &min_loc, &max_loc);
  height_map -= min;
  height_map /= (max - min);
  cv::pow(height_map, 4.f, height_map);

  int water_size = 1024;
  cv::Mat water_map(water_size, water_size, CV_32FC1, cv::Scalar(0));

  for (int x = 0; x < water_size; x++) {
    for (int y = 0; y < water_size; y++) {
      glm::vec2 coord = glm::vec2(x, y) * 0.01f;
      float amp = 1.f;
      float frequ = 1.f;
      float amp_sum = 0.f;
      for (int i = 0; i < 6; i++) {
        water_map.at<float>(y, x) += amp * (1.f - abs(noise_gen.getPerlin(coord * frequ)));
        amp_sum += amp;
        amp *= 0.35f;
        frequ *= 2.25f;
      }
      water_map.at<float>(y, x) /= amp_sum;
      water_map.at<float>(y, x) = pow(water_map.at<float>(y, x), 5.f);
    }
  }

  //cv::imshow("water map", water_map);
  //cv::waitKey();
 // cv::imwrite("assets/water_map.png", water_map * 255.f);

//  cv::Mat splat_map = cv::imread("assets/splat_map.png");
 // cv::cvtColor(splat_map, splat_map, CV_BGR2GRAY);
 // splat_map.convertTo(splat_map, CV_32FC1, 1.0 / 255.0);

  cv::Mat splat_map(height_map.size(), CV_32FC4, cv::Scalar(0, 0, 0, 0));

  float grass_bound = 0.3f;
  for (int x = 0; x < splat_map.cols; x++) {
    for (int y = 0; y < splat_map.rows; y++) {
      float height = height_map.at<float>(y, x);
      if (height <= grass_bound) {
        splat_map.at<cv::Vec4f>(y, x)[0] = 1.f - glm::smoothstep(0.f, grass_bound, height);
        splat_map.at<cv::Vec4f>(y, x)[1] = glm::smoothstep(0.f, grass_bound, height);
      }
      else {
        splat_map.at<cv::Vec4f>(y, x)[1] = 1.f;
      }
    }
  }

  // roads
  float lower_bound = 0.05f;
  float upper_bound = 0.07f;
  for (int x = 0; x < image_size; x++) {
    for (int y = 0; y < image_size; y++) {
      float val = height_map.at<float>(y, x);
      if (val >= lower_bound && val <= upper_bound) {
        float factor = pow(glm::smoothstep(lower_bound, upper_bound, val), 2.f);
        height_map.at<float>(y, x) = glm::mix(lower_bound, upper_bound, factor);
        factor = pow(factor, 0.3f);
        auto& splat_val = splat_map.at<cv::Vec4f>(y, x);
        splat_val[0] = 0.f;
        splat_val[1] = factor;
        splat_val[2] = 1.f - factor;
      }
    }
  }

  lower_bound = 0.02f;
  upper_bound = 0.0275f;
  for (int x = 0; x < image_size; x++) {
    for (int y = 0; y < image_size; y++) {
      float val = height_map.at<float>(y, x);
      if (val >= lower_bound && val <= upper_bound) {
        float factor = pow(glm::smoothstep(lower_bound, upper_bound, val), 32.f);
        height_map.at<float>(y, x) = glm::mix(lower_bound, upper_bound, factor);
       // factor = pow(factor, 0.5f);
        factor = pow(factor, 0.1f);
        auto& splat_val = splat_map.at<cv::Vec4f>(y, x);
        splat_val[0] = 1.f - factor;
        splat_val[1] = factor;
      }
    }
  }

  lower_bound = 0.01f;
  upper_bound = 0.015f;
  for (int x = 0; x < image_size; x++) {
    for (int y = 0; y < image_size; y++) {
      float val = height_map.at<float>(y, x);
      if (val >= lower_bound && val <= upper_bound) {
        float factor = pow(glm::smoothstep(lower_bound, upper_bound, val), 32.f);
        height_map.at<float>(y, x) = glm::mix(lower_bound, upper_bound, factor);
        // factor = pow(factor, 0.5f);
        factor = pow(factor, 0.1f);
        auto& splat_val = splat_map.at<cv::Vec4f>(y, x);
        splat_val[0] = 1.f - factor;
        splat_val[1] = factor;
      }
    }
  }

  // lakes
  float water_bound = 0.005f;
  for (int x = 0; x < height_map.cols; x++) {
    for (int y = 0; y < height_map.rows; y++) {
      float height = height_map.at<float>(y, x);
      if (height <= water_bound) {
        float factor = pow(1.f - glm::smoothstep(0.f, water_bound, height), 0.5f);
        height_map.at<float>(y, x) -= 0.08f * factor;
      }
    }
  }

  // splat map weight normalization
  for (int x = 0; x < splat_map.cols; x++) {
    for (int y = 0; y < splat_map.rows; y++) {
      auto& val = splat_map.at<cv::Vec4f>(y, x);
      float sum = val[0] + val[1] + val[2] + val[3];
      if (sum != 0.f) {
        val /= sum;
      }
    }
  }

  height_map *= 255.f;
  cv::imwrite("assets/height_map.png", height_map);
  geo_mip_map->setHeightMap(height_map);
  geo_mip_map->generateTiles();
  geo_mip_map->setSplatMapImage(splat_map);

  auto tree_model_lod0 = _importer->loadModel("assets/tree_lod0.obj");
  geo_mip_map->setTreeModelLod0(tree_model_lod0);
  auto tree_model_lod1 = _importer->loadModel("assets/tree_lod1.obj");
  geo_mip_map->setTreeModelLod1(tree_model_lod1);
  auto leaves_model = _importer->loadModel("assets/leafs_lod1.obj");
  geo_mip_map->setLeavesModel(leaves_model);

  float trees_per_dir = 500;
  float terrain_size = height_map.cols;
  float delta = terrain_size / trees_per_dir;
  std::mt19937 gen;
  std::uniform_real_distribution<float> dist(-terrain_size * 0.005f, terrain_size * 0.005f);
  std::uniform_real_distribution<float> scale_dist(0.4f, 0.65f);
  std::uniform_real_distribution<float> rot_dist(0.f, glm::pi<float>());
  for (float x = delta; x < terrain_size - delta; x += delta) {
    for (float y = delta; y < terrain_size - delta; y += delta) {
      glm::vec2 xz(x + dist(gen), y + dist(gen));
      xz = glm::clamp(xz, glm::vec2(0), glm::vec2(image_size - 1));
      float height = geo_mip_map->getHeight(xz.x, xz.y);
      float grass_density = splat_map.at<cv::Vec4f>(xz.y, xz.x)[0];
      if (height / 255.f < 0.25f && grass_density >= 0.4f && height / 255.f >= water_bound + 0.002f) {
        glm::vec3 translation(xz.x, height - 0.5f, xz.y);
        float scale = scale_dist(gen);
        auto transform = glm::translate(translation) * glm::rotate(rot_dist(gen), glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(scale));
        geo_mip_map->addTree(transform, scale);
      }
    }
  }

  float cloud_billboards_per_dir = 200;
  delta = terrain_size / cloud_billboards_per_dir;
  for (float x = delta; x < terrain_size - delta; x += delta) {
    for (float y = delta; y < terrain_size - delta; y += delta) {
      glm::vec2 xz(x + dist(gen), y + dist(gen));
      xz = glm::clamp(xz, glm::vec2(0), glm::vec2(image_size - 1));
      float height = geo_mip_map->getHeight(xz.x, xz.y);
      if (height / 255.f > 0.4f) {
        glm::vec3 pos(xz.x, height, xz.y);
        float scale = 200.f * scale_dist(gen);
        geo_mip_map->addCloudBillboard(pos, scale);
      }
    }
  }

  geo_mip_map->build();

  _geoMipMapEntity->addComponent(geo_mip_map);
  auto terrain_transform = std::make_shared<fly::Transform>(glm::vec3(0.f, -250.f, 0.f), glm::vec3(4.f));
  _geoMipMapEntity->addComponent(terrain_transform);
#endif

  auto camera = _engine->getEntityManager()->createEntity();
  camera->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 2.5f, 0.f), glm::vec3(0.f, 0.f, 0.f)));
  _cameras.push_back(camera);

  _lights.push_back(_engine->getEntityManager()->createEntity());
  _sphereModel = _importer->loadModel("assets/sphere.obj");
  _rs->setSkydome(_sphereModel->getMeshes()[0]);
  auto dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(200.f), _csmDistances);
 // auto transform = std::make_shared<fly::Transform>(glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(200.f));
 // dl->_target = glm::vec3(0.f);
  _lights.back()->addComponent(dl);
  std::shared_ptr<fly::Light> light_base = dl;
  _lights.back()->addComponent(light_base);
//  _lights.back()->addComponent(transform);
 // _lights.back()->addComponent(_sphereModel);

  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0); // Render as fast as possible

  timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, &GLWidget::updateLabels);
  timer->start(1000); // Update labels 1 times per second

  _timePoint = _timePointStart;
  _gameTimer = fly::GameTimer();
}

void GLWidget::paintGL()
{
  _gameTimer.tick();
  _deltaTime = _gameTimer.getDeltaTimeSeconds();
  _time = _gameTimer.getTimeSeconds();
  _frameCounter++;

  if (_guiLeft->isHidden()) {
    auto c = cursor();
    _mouseDelta = !_guiWasHidden ? glm::vec2(0.f) : glm::vec2(c.pos().x(), c.pos().y()) - glm::vec2(width() / 2, height() / 2);
    c.setPos(QPoint(width() / 2, height() / 2));
    setCursor(c);
    handleKeyEvents();
  }
  _guiWasHidden = _guiLeft->isHidden();

  _rs->setDefaultFramebufferId(defaultFramebufferObject());
  _engine->update(_time, _deltaTime);
}

void GLWidget::resizeGL(int width, int height)
{
  _rs->onResize(glm::vec2(width, height));
  float font_size = height * 0.0125f;
  _font.setPointSizeF(font_size);
  setFont(_font);
  _fontBig = QFont(_font);
  _fontBig.setPointSizeF(font_size * 1.5f);
  _guiLeft->setFont(_fontBig);
 // _colorDialog->setFont(_font);
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
}

void GLWidget::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());

  if (e->key() == Qt::Key_Escape) {
    if (_guiLeft->isHidden()) {
      showGui();
    }
    else {
      hideGui();
    }
  }
  if (_guiLeft->isHidden()) {
    if (e->key() == Qt::Key_1) {
      _rs->setRenderWireFrame(!_rs->getRenderWireFrame());
      _rs->_smoothCamera = !_rs->getRenderWireFrame();
    }
    else if (e->key() == Qt::Key_2) {
      _rs->_ssrEnabled = !_rs->_ssrEnabled;
    }
    else if (e->key() == Qt::Key_7) {
      _rs->_useTreeBillboards = !_rs->_useTreeBillboards;
    }
    else if (e->key() == Qt::Key_K) {
      _mode = Mode::CAMERA;
    }

    else if (e->key() == Qt::Key_L) {
      _mode = Mode::LIGHT;
    }

    else if (e->key() == Qt::Key_M) {
      _mode = Mode::LIGHT_MOVE;
    }

    else if (e->key() == Qt::Key_F) {
      _rs->_lensFlaresEnabled = !_rs->_lensFlaresEnabled;
    }

    else if (e->key() == Qt::Key_Tab) {
      _rs->setRenderAABBs(!_rs->getRenderAABBs());
    }

    else if (_mode == Mode::CAMERA && e->key() == Qt::Key_Insert) {
      auto camera = _engine->getEntityManager()->createEntity();
      auto cam = std::make_shared<fly::Camera>(_cameras.front()->getComponent<fly::Camera>()->_pos, _cameras.front()->getComponent<fly::Camera>()->_eulerAngles);
      cam->_isActive = false;
      camera->addComponent(cam);
      _cameras.push_back(camera);
    }

    if (_mode == Mode::CAMERA && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right)) {
      if (e->key() == Qt::Key_Left) {
        std::rotate(_cameras.begin(), _cameras.begin() + 1, _cameras.end());
      }
      else {
        std::rotate(_cameras.begin(), _cameras.end() - 1, _cameras.end());
      }
      for (auto& c : _cameras) {
        c->getComponent<fly::Camera>()->_isActive = false;
      }
      _cameras.front()->getComponent<fly::Camera>()->_isActive = true;
    }

    if (_mode == Mode::LIGHT && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) && _lights.back()->getComponent<fly::Animation>() == nullptr) {
      if (e->key() == Qt::Key_Left) {
        std::rotate(_lights.begin(), _lights.begin() + 1, _lights.end());
      }
      else if (e->key() == Qt::Key_Right) {
        std::rotate(_lights.begin(), _lights.end() - 1, _lights.end());
      }
      auto scale_start = _lights.back()->getComponent<fly::Transform>()->getScale();
      auto scale_end = scale_start * 2.f;
      auto animation = std::make_shared<fly::Animation>(0.3f, _time, [scale_start, scale_end, this](float t) {
        t = t <= 0.5f ? t * 2.f : (1.f - t) * 2.f;
       // _lights.back()->getComponent<fly::Transform>()->getScale() = (1.f - t) * scale_start + t * scale_end;
      });
      _lights.back()->addComponent(animation);
    }


    if (_mode == Mode::LIGHT && e->key() == Qt::Key_Delete && _lights.size()) {
      auto l = _lights.front();
      _engine->getEntityManager()->removeEntity(l.get());
      _lights.erase(_lights.begin());
    }
    if (_mode == Mode::CAMERA && e->key() == Qt::Key_Delete && _cameras.size() > 1) {
      auto c = _cameras.front();
      _engine->getEntityManager()->removeEntity(c.get());
      _cameras.erase(_cameras.begin());
    }
    if (e->key() == Qt::Key_I) {
      _rs->initShaders();
    }
  }
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    _buttonsPressed.insert(Qt::MouseButton::LeftButton);
    if (_lights.back() != nullptr) {
      _lightDistWhenClicked = glm::distance(glm::vec3(_lights.back()->getComponent<fly::Light>()->_pos), _cameras.front()->getComponent<fly::Camera>()->_pos);
    }
  }
  if (e->button() == Qt::MouseButton::RightButton) {
    _buttonsPressed.insert(Qt::MouseButton::RightButton);
  }
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    _buttonsPressed.erase(Qt::MouseButton::LeftButton);
  }
  if (e->button() == Qt::MouseButton::RightButton) {
    _buttonsPressed.erase(Qt::MouseButton::RightButton);
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
  for (int i = _lights.size() - 1; i >= 0; i--) {
    auto sl = _lights[i]->getComponent<fly::SpotLight>();
    if (sl != nullptr) {
      sl->_umbraDegrees += static_cast<float>(e->delta()) / 100.f;
      sl->_penumbraDegrees += static_cast<float>(e->delta()) / 100.f;
      break;
    }
  }
}

void GLWidget::initGui()
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  _fpsLabel = new QLabel("");
  int id = QFontDatabase::addApplicationFont("assets/Roboto-Bold.ttf");
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  _font = QFont(family);
  _fpsLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
  /* auto c = cursor();
   c.setShape(Qt::CursorShape::BlankCursor);
   setCursor(c);*/

  _guiWidget = new QWidget();
  _guiWidget->setAutoFillBackground(true);

  QPalette pal;
  pal.setColor(QPalette::Background, _guiBackgroundColor);
  pal.setColor(QPalette::Foreground, Qt::white);
  _guiWidget->setPalette(pal);

  layout->addWidget(_guiWidget);

  QHBoxLayout* gui_layout = new QHBoxLayout(_guiWidget);
  _guiLeft = new QWidget();
  gui_layout->addWidget(_guiLeft, 2);
  QVBoxLayout* gui_left_layout = new QVBoxLayout(_guiLeft);
  QLabel* label = new QLabel("Settings");
  gui_left_layout->addWidget(label);
  QWidget* bloom_widget = new QWidget();
  QHBoxLayout* bloom_layout = new QHBoxLayout(bloom_widget);
  QCheckBox* check_box = new QCheckBox("Bloom");
  check_box->setChecked(_rs->isBloomEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setBloomEnabled(state);
  });
  QSlider* bloom_slider_small = new QSlider(Qt::Horizontal);
  bloom_slider_small->setRange(0, 5000);
  bloom_slider_small->setValue(_rs->getBloomWeights()[0] * 1000);
  QObject::connect(bloom_slider_small, &QSlider::valueChanged, this, [bloom_slider_small, this](int value) {
    float val = static_cast<float>(value) / 1000.f;
    _rs->setBloomWeight(0, val);
  });
  QSlider* bloom_slider_medium = new QSlider(Qt::Horizontal);
  bloom_slider_medium->setRange(0, 5000);
  bloom_slider_medium->setValue(_rs->getBloomWeights()[1] * 1000);
  QObject::connect(bloom_slider_medium, &QSlider::valueChanged, this, [bloom_slider_medium, this](int value) {
    float val = static_cast<float>(value) / 1000.f;
    _rs->setBloomWeight(1, val);
  });
  QSlider* bloom_slider_large = new QSlider(Qt::Horizontal);
  bloom_slider_large->setRange(0, 5000);
  bloom_slider_large->setValue(_rs->getBloomWeights()[2] * 1000);
  QObject::connect(bloom_slider_large, &QSlider::valueChanged, this, [bloom_slider_large, this](int value) {
    float val = static_cast<float>(value) / 1000.f;
    _rs->setBloomWeight(2, val);
  });
  bloom_layout->addWidget(check_box);
  bloom_layout->addWidget(bloom_slider_small);
  bloom_layout->addWidget(bloom_slider_medium);
  bloom_layout->addWidget(bloom_slider_large);
  gui_left_layout->addWidget(bloom_widget);

  QWidget* light_vol_widget = new QWidget();
  QHBoxLayout* light_vol_layout = new QHBoxLayout(light_vol_widget);
  check_box = new QCheckBox("Light volumes");
  check_box->setChecked(_rs->isLightVolumesEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setLightVolumesEnabled(state);
  });
  QSlider* light_vol_slider_samples = new QSlider(Qt::Horizontal);
  light_vol_slider_samples->setRange(1, 256);
  light_vol_slider_samples->setValue(_rs->getVolumeLightSamples());
  QObject::connect(light_vol_slider_samples, &QSlider::valueChanged, this, [this](int value) {
    _rs->setVolumeLightSamples(value);
  });
  QSlider* light_vol_slider_weight = new QSlider(Qt::Horizontal);
  light_vol_slider_weight->setRange(0, 1000);
  light_vol_slider_weight->setValue(_rs->getVolumeLightWeight() * 1000);
  QObject::connect(light_vol_slider_weight, &QSlider::valueChanged, this, [this](int value) {
    _rs->setVolumeLightWeight(value / 1000.f);
  });
  light_vol_layout->addWidget(check_box);
  light_vol_layout->addWidget(light_vol_slider_samples);
  light_vol_layout->addWidget(light_vol_slider_weight);
  gui_left_layout->addWidget(light_vol_widget);
  QWidget* shadows_widget = new QWidget();
  QHBoxLayout* shadows_layout = new QHBoxLayout(shadows_widget);
  check_box = new QCheckBox("Shadows");
  check_box->setChecked(_rs->isShadowsEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setShadowsEnabled(state);
  });
  shadows_layout->addWidget(check_box);
  QSlider* shadow_slider_pcf_samples = new QSlider(Qt::Horizontal);
  shadow_slider_pcf_samples->setRange(1, 16);
  shadow_slider_pcf_samples->setValue(_rs->getShadowPCFSamples());
  QObject::connect(shadow_slider_pcf_samples, &QSlider::valueChanged, this, [this](int value) {
    _rs->setShadowPCFSamples(value);
  });
  shadows_layout->addWidget(shadow_slider_pcf_samples);
  gui_left_layout->addWidget(shadows_widget);
  check_box = new QCheckBox("God Rays");
  check_box->setChecked(_rs->isGodRaysEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setGodRaysEnabled(state);
  });
  gui_left_layout->addWidget(check_box);
  QWidget* dof_widget = new QWidget();
  QHBoxLayout* dof_layout = new QHBoxLayout(dof_widget);
  check_box = new QCheckBox("Depth Of Field");
  check_box->setChecked(_rs->isDepthOfFieldEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setDepthOfFieldEnabled(state);
  });
  dof_layout->addWidget(check_box);
  QSlider* dof_near_slider = new QSlider(Qt::Horizontal);
  dof_near_slider->setRange(0, 300);
  dof_near_slider->setValue(_rs->getDofParam().x * 10.f);
  QObject::connect(dof_near_slider, &QSlider::valueChanged, this, [this](int value) {
    auto param = _rs->getDofParam();
    param.x = value / 10.f;
    _rs->setDofParam(param);
  });
  dof_layout->addWidget(dof_near_slider);
  QSlider* dof_focus_slider = new QSlider(Qt::Horizontal);
  dof_focus_slider->setRange(3, 100);
  dof_focus_slider->setValue(_rs->getDofParam().y);
  QObject::connect(dof_focus_slider, &QSlider::valueChanged, this, [this](int value) {
    auto param = _rs->getDofParam();
    param.y = value;
    _rs->setDofParam(param);
  });
  dof_layout->addWidget(dof_focus_slider);
  QSlider* dof_far_slider = new QSlider(Qt::Horizontal);
  dof_far_slider->setRange(5, 20000);
  dof_far_slider->setValue(_rs->getDofParam().z);
  QObject::connect(dof_far_slider, &QSlider::valueChanged, this, [this](int value) {
    auto param = _rs->getDofParam();
    param.z = value;
    _rs->setDofParam(param);
  });
  dof_layout->addWidget(dof_far_slider);
  gui_left_layout->addWidget(dof_widget);
  check_box = new QCheckBox("Frustum culling");
  check_box->setChecked(_rs->isFrustumCullingEnabled());
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    _rs->setFrustumCullingEnabled(state);
  });
  gui_left_layout->addWidget(check_box);
  check_box = new QCheckBox("Fullscreen");
  check_box->setChecked(false);
  QObject::connect(check_box, &QCheckBox::stateChanged, this, [this](int state) {
    if (state) {
      showFullScreen();
    }
    else {
      showNormal();
    }
  });
  gui_left_layout->addWidget(check_box);

  QWidget* fov_widget = new QWidget();
  QHBoxLayout* fov_layout = new QHBoxLayout(fov_widget);
  auto fov_label = new QLabel("FOV");
  fov_layout->addWidget(fov_label);
  QSlider* fov_slider = new QSlider(Qt::Horizontal);
  fov_slider->setRange(20, 180);
  fov_slider->setValue(_rs->_fov);
  QObject::connect(fov_slider, &QSlider::valueChanged, this, [this](int value) {
    _rs->_fov = value;
  });
  fov_layout->addWidget(fov_slider);
  gui_left_layout->addWidget(fov_widget);

 /* QWidget* light_widget = new QWidget();
  QHBoxLayout* light_layout = new QHBoxLayout(light_widget);
  QWidget* radio_button_widget = new QWidget();
  QVBoxLayout* radio_button_layout = new QVBoxLayout(radio_button_widget);
  light_layout->addWidget(radio_button_widget);
  auto spot_radio_button = new QRadioButton("Spot");
  auto point_radio_button = new QRadioButton("Point");
  auto directional_radio_button = new QRadioButton("Directional");
  directional_radio_button->setChecked(true);
  radio_button_layout->addWidget(directional_radio_button);
  radio_button_layout->addWidget(spot_radio_button);
  radio_button_layout->addWidget(point_radio_button);
  _colorDialog = new QColorDialog();
  light_layout->addWidget(_colorDialog);
  ClickableLabel* light_label = new ClickableLabel("Add light");
  light_layout->addWidget(light_label);
  gui_left_layout->addWidget(light_widget);
  QObject::connect(light_label, &ClickableLabel::clicked, [this, directional_radio_button, spot_radio_button, point_radio_button]() {
    makeCurrent();
    auto light = fly::EntityManager::instance().createEntity();
    _lights.push_back(light);
    auto col = _colorDialog->currentColor();
    glm::vec3 col_scaled(col.red(), col.green(), col.blue());
    col_scaled = col_scaled / 255.f * 1.75f;
    auto cam = _cameras.front()->getComponent<fly::Camera>();
    auto pos = cam->_pos + cam->_direction * 2.f;
    auto target = pos + glm::vec3(0.f, -1.f, 0.f);
    _lightDistWhenClicked = glm::distance(pos, cam->_pos);
    std::shared_ptr<fly::Animation> animation;
    auto transform = std::make_shared<fly::Transform>(pos, glm::vec3(0.05f));
    _lights.back()->addComponent(transform);
    auto light_model = std::make_shared<fly::Model>(*_sphereModel);
    _lights.back()->addComponent(light_model);
    if (point_radio_button->isChecked()) {
      auto pl = std::make_shared<fly::PointLight>(col_scaled, 0.1f, 15.f);
      _lights.back()->addComponent(pl);
      glm::vec3 scale_start(0.f);
      glm::vec3 scale_end = transform->getScale();
      animation = std::make_shared<fly::Animation>(0.8f, _time, [pl, transform, scale_start, scale_end, col_scaled](float t) {
        transform->getScale() = (1.f - t) * scale_start + t * scale_end;
        pl->_color = (1.f - t) * glm::vec3(0.f) + t * col_scaled;
      });
    }
    else if (directional_radio_button->isChecked()) {
      auto dl = std::make_shared<fly::DirectionalLight>(col_scaled, _csmDistances);
      _lights.back()->addComponent(dl);
      glm::vec3 scale_start(0.f);
      glm::vec3 scale_end = transform->getScale();
      animation = std::make_shared<fly::Animation>(0.8f, _time, [dl, transform, scale_start, scale_end, col_scaled](float t) {
        transform->getScale() = (1.f - t) * scale_start + t * scale_end;
        dl->_color = (1.f - t) * glm::vec3(0.f) + t * col_scaled;
      });
    }
    else if (spot_radio_button->isChecked()) {
      auto sl = std::make_shared<fly::SpotLight>(col_scaled, 0.1f, 15.f, 20.f, 24.f);
      sl->_target = target;
      _lights.back()->addComponent(sl);
      glm::vec3 scale_start(0.f);
      glm::vec3 scale_end = transform->getScale();
      float umbra_start, penumbra_start = 0.f;
      float umbra_end = sl->_umbraDegrees;
      float penumbra_end = sl->_penumbraDegrees;
      animation = std::make_shared<fly::Animation>(0.8f, _time, [sl, transform, scale_start, scale_end, umbra_start, umbra_end, penumbra_start, penumbra_end, col_scaled](float t) {
        transform->getScale() = (1.f - t) * scale_start + t * scale_end;
        sl->_umbraDegrees = (1.f - t) * umbra_start + t * umbra_end;
        sl->_penumbraDegrees = (1.f - t) * penumbra_start + t * penumbra_end;
        sl->_color = (1.f - t) * glm::vec3(0.f) + t * col_scaled;
      }, std::make_shared<fly::Animation::OvershootInterpolator>());
    }
    _lights.back()->addComponent(animation);
    hideGui();
  });*/

#if PROFILE
  auto profiling_widget = new QWidget();
  auto profiling_layout = new QGridLayout();

  profiling_layout->addWidget(new QLabel("Trees"), 0, 0);
  _lblTreeDrawCalls = new QLabel();
  profiling_layout->addWidget(_lblTreeDrawCalls, 1, 0);
  _lblTreesMs = new QLabel();
  profiling_layout->addWidget(_lblTreesMs, 2, 0);
  _lblTreesShadowMapMs = new QLabel();
  profiling_layout->addWidget(_lblTreesShadowMapMs, 3, 0);
  _lblTreeTriangles = new QLabel();
  profiling_layout->addWidget(_lblTreeTriangles, 4, 0);
  _lblTreeShadowMapTriangles = new QLabel();
  profiling_layout->addWidget(_lblTreeShadowMapTriangles, 5, 0);

  profiling_layout->addWidget(new QLabel("Grass"), 0, 1);
  _lblGrassDrawCalls = new QLabel();
  profiling_layout->addWidget(_lblGrassDrawCalls, 1, 1);
  _lblGrassMs = new QLabel();
  profiling_layout->addWidget(_lblGrassMs, 2, 1);
  _lblGrassBlades = new QLabel();
  profiling_layout->addWidget(_lblGrassBlades, 3, 1);
  _lblGrassTriangles = new QLabel();
  profiling_layout->addWidget(_lblGrassTriangles, 4, 1);

  profiling_widget->setLayout(profiling_layout);
  gui_left_layout->addWidget(profiling_widget);
#endif

  ClickableLabel* quit_button = new ClickableLabel("Quit");
  QObject::connect(quit_button, &ClickableLabel::clicked, &QApplication::quit);
  gui_left_layout->addWidget(quit_button);
  // gui_left_layout->addStretch();
  // _guiLeft->hide();

  QWidget* gui_right = new QWidget();
  gui_layout->addWidget(gui_right, 1);
  QHBoxLayout* gui_right_layout = new QHBoxLayout(gui_right);
  gui_right_layout->addWidget(_fpsLabel);
}

void GLWidget::showGui()
{
  _guiLeft->show();
  _guiLeft->setEnabled(true);
  QPalette pal = _guiWidget->palette();
  pal.setColor(QPalette::Background, _guiBackgroundColor);
  _guiWidget->setPalette(pal);
  auto c = cursor();
  c.setShape(Qt::CursorShape::ArrowCursor);
  setCursor(c);
  _gameTimer.stop();
}

void GLWidget::hideGui()
{
  _guiLeft->hide();
  _guiLeft->setEnabled(false);
  QPalette pal = _guiWidget->palette();
  pal.setColor(QPalette::Background, Qt::transparent);
  _guiWidget->setPalette(pal);
  auto c = cursor();
  c.setShape(Qt::CursorShape::BlankCursor);
  setCursor(c);
  _gameTimer.start();
}

void GLWidget::handleKeyEvents()
{
#if SPONZA
  float cam_speed = 10.f * _deltaTime;
#else
  float cam_speed = 30.f * _deltaTime;
#endif

  if (_keysPressed.find(Qt::Key_Shift) != _keysPressed.end()) {
    cam_speed *= 3.f;
  }
  else if (_keysPressed.find(Qt::Key_Control) != _keysPressed.end()) {
    cam_speed *= 0.5f;
  }
  auto cam = _cameras.front()->getComponent<fly::Camera>();
  //std::cout << glm::to_string(cam->_pos) << std::endl;
  if (_keysPressed.find(Qt::Key_W) != _keysPressed.end()) {
    cam->_pos += cam->_direction * cam_speed;
  }
  if (_keysPressed.find(Qt::Key_A) != _keysPressed.end()) {
    cam->_pos -= cam->_right * cam_speed;
  }
  if (_keysPressed.find(Qt::Key_S) != _keysPressed.end()) {
    cam->_pos -= cam->_direction * cam_speed;
  }
  if (_keysPressed.find(Qt::Key_D) != _keysPressed.end()) {
    cam->_pos += cam->_right * cam_speed;
  }
  if (_keysPressed.find(Qt::Key_Space) != _keysPressed.end()) {
    cam->_pos += cam->_up * cam_speed;
  }
  if (_keysPressed.find(Qt::Key_C) != _keysPressed.end()) {
    cam->_pos -= cam->_up * cam_speed;
  }
#if !SPONZA
  auto geo_mip_map = _geoMipMapEntity->getComponent<fly::Terrain>();
  auto g_model_mat = _geoMipMapEntity->getComponent<fly::Transform>()->getModelMatrix();
  auto cam_pos_terrain = glm::mat4(inverse(g_model_mat)) * glm::vec4(cam->_pos, 1.f);
  if (cam_pos_terrain.x >= 0.f && cam_pos_terrain.x <= geo_mip_map->getHeightMap().cols && cam_pos_terrain.z >= 0.f && cam_pos_terrain.z <= geo_mip_map->getHeightMap().rows) {
    float height = geo_mip_map->getHeight(cam_pos_terrain.x, cam_pos_terrain.z);
    float height_world = (glm::mat4(g_model_mat) * glm::vec4(0.f, height, 0.f, 1.f)).y;
    cam->_pos.y = std::max(height_world + _rs->_zNear * 2.f, cam->_pos.y);
  }
#endif
  glm::vec3 mouse_move(_mouseDelta.x, _mouseDelta.y, 0.f);
  float roll_speed = 15.f;
  if (_keysPressed.find(Qt::Key_E) != _keysPressed.end()) {
    mouse_move.z += roll_speed;
  }
  if (_keysPressed.find(Qt::Key_Q) != _keysPressed.end()) {
    mouse_move.z -= roll_speed;
  }
  float mouse_speed = 0.09f * _deltaTime;
  if (_rs->getRenderWireFrame()) {
    mouse_speed *= 10.f;
  }
  cam->_eulerAngles -= mouse_move * mouse_speed;

  glm::vec3 euler_target(cam->_eulerAngles.x, cam->_eulerAngles.y, 0.f);
  glm::quat target_quat(euler_target);
  glm::quat current_quat(cam->_eulerAngles);
  auto interpolated_quat = glm::slerp(target_quat, current_quat, 0.99f);
  cam->_eulerAngles = glm::eulerAngles(interpolated_quat);

  if (_lights.size() && _mode == Mode::LIGHT_MOVE) {
   // auto& light_translation = _lights.back()->getComponent<fly::Transform>()->getTranslation();
    auto light = _lights.back()->getComponent<fly::Light>();
    float light_speed = 100.f * _deltaTime;
    if (_keysPressed.find(Qt::Key_Up) != _keysPressed.end()) {
      light->_pos[0] += light_speed;
    }
    if (_keysPressed.find(Qt::Key_Down) != _keysPressed.end()) {
      light->_pos[0] -= light_speed;
    }
    if (_keysPressed.find(Qt::Key_Right) != _keysPressed.end()) {
      light->_pos[2] += light_speed;
    }
    if (_keysPressed.find(Qt::Key_Left) != _keysPressed.end()) {
      light->_pos[2] -= light_speed;
    }
    float alpha = 0.99f;
    if (_buttonsPressed.find(Qt::MouseButton::LeftButton) != _buttonsPressed.end()) {
      light->_pos = (1.f - alpha) * (cam->_pos + cam->_direction * _lightDistWhenClicked) + glm::vec3(light->_pos * alpha);
    }
    if (_buttonsPressed.find(Qt::MouseButton::RightButton) != _buttonsPressed.end()) {
      float scene_depth = _rs->getSceneDepth(glm::ivec2(width() / 2, height() / 2));
      auto vp_inverse = inverse(_rs->getProjectionMatrix() * _rs->getViewMatrix());
      auto pos_world_space = vp_inverse * glm::vec4(glm::vec3(0.5f, 0.5f, scene_depth) * 2.f - 1.f, 1.f);
      pos_world_space /= pos_world_space.w;
      _lightTargetWorld = (1.f - alpha) * glm::vec3(pos_world_space) + alpha * _lightTargetWorld;
      auto sl = _lights.back()->getComponent<fly::SpotLight>();
      auto dl = _lights.back()->getComponent<fly::DirectionalLight>();
      if (sl != nullptr) {
        sl->_target = _lightTargetWorld;
      }
      else if (dl != nullptr) {
        dl->_target = _lightTargetWorld;
      }
    }
  }
}

void GLWidget::updateLabels()
{
  _fpsLabel->setText(QString::fromStdString(std::to_string(_frameCounter) + " FPS"));
  _frameCounter = 0;
#if PROFILE
  _lblTreeDrawCalls->setText(QString::fromStdString("Draw calls:" + std::to_string(_rs->getTreeDrawCalls())));
  _lblTreesMs->setText(QString::fromStdString("Render time:" + std::to_string(_rs->getTiming(fly::RenderingSystem::RenderStage::TREES).getElapsedMillis())));
  _lblTreesShadowMapMs->setText(QString::fromStdString("Render time shadow map:" + std::to_string(_rs->getTiming(fly::RenderingSystem::RenderStage::TREES_SHADOW_MAP).getElapsedMillis())));
  _lblTreeTriangles->setText(QString::fromStdString("Triangles:" + std::to_string(_rs->getTreeTriangles())));
  _lblTreeShadowMapTriangles->setText(QString::fromStdString("Triangles shadow map:" + std::to_string(_rs->getTreeTrianglesShadowMap())));

  _lblGrassDrawCalls->setText(QString::fromStdString("Draw calls:" + std::to_string(_rs->getGrassDrawCalls())));
  _lblGrassMs->setText(QString::fromStdString("Render time:" + std::to_string(_rs->getTiming(fly::RenderingSystem::RenderStage::GRASS).getElapsedMillis())));
  _lblGrassBlades->setText(QString::fromStdString("Blades:" + std::to_string(_rs->getGrassBlades())));
  _lblGrassTriangles->setText(QString::fromStdString("Triangles:" + std::to_string(_rs->getGrassTriangles())));
#endif
}

float GLWidget::bernstein(float u, int i, int n)
{
  return binomial(i, n) * pow(u, i) * pow(1.f - u, n - i);
}

int GLWidget::binomial(int i, int n)
{
  return factorial(n) / (factorial(i) * factorial(n - i));
}

int GLWidget::factorial(int n)
{
  if (n == 0) {
    return 1;
  }

  for (int i = n - 1; i > 0; i--) {
    n *= i;
  }
  return n;
}

glm::vec2 GLWidget::deCasteljau(float u, const std::vector<glm::vec2>& points, glm::vec2& tangent)
{
  std::vector<glm::vec2> new_points;
  for (int i = 0; i < points.size() - 1; i++) {
    new_points.push_back(glm::mix(points[i], points[i + 1], u));
  }
  if (new_points.size() == 1) {
    tangent = points[1] - points[0];
    return new_points.front();
  }
  else {
    return deCasteljau(u, new_points, tangent);
  }
}

cv::Mat GLWidget::generateSplatMap(int size)
{
  cv::Mat splat_map (size, size, CV_32FC1, cv::Scalar(255));

  glm::vec2 p1(0, 0);
  glm::vec2 p2(size, size);

  std::mt19937 gen;
  std::uniform_real_distribution<float> dist(-1.f, 1.f);

  generateSplatMap(p1, p2, gen, dist, splat_map);

  cv::GaussianBlur(splat_map, splat_map, cv::Size(3, 3), 0, 0);
  cv::imwrite("assets/splat_map.png", splat_map);

  return splat_map;
}

void GLWidget::generateSplatMap(const glm::vec2& p1, const glm::vec2& p2, std::mt19937& gen, std::uniform_real_distribution<float>& dist, cv::Mat& splat_map)
{
  auto cv_point = [](const glm::vec2& v) {
    return cv::Point(v.x, v.y);
  };

  int num_control_points = 10;

  glm::vec2 ray = p2 - p1;
  glm::vec2 delta = ray / static_cast<float>(num_control_points - 1);

 // std::cout << length(ray) << std::endl;

  float offset_scale = 0.1f * length(ray);

  std::vector<glm::vec2> ctrl_points(num_control_points);
  ctrl_points[0] = p1;
  ctrl_points.back() = p2;

  glm::vec2 pos = p1 + delta;
  for (int i = 1; i < num_control_points - 1; i++, pos += delta) {
    ctrl_points[i] = pos + glm::vec2(dist(gen), dist(gen)) * offset_scale;
  }

  /*for (auto& c : ctrl_points) {
    cv::circle(splat_map, to_cv_point(c), 10, cv::Scalar(255));
  }
  cv::line(splat_map, to_cv_point(p1), to_cv_point(p2), cv::Scalar(255));*/

  int num_samples = 100;
  std::vector<glm::vec2> points_on_curve;

  //std::vector<glm::vec2> branch_normals;

  std::vector<glm::vec2> branches_p1;
  std::vector<glm::vec2> branches_p2;

  for (int i = 0; i <= num_samples - 1; i++) 
  {
    float u = i / static_cast<float>(num_samples - 1);
    glm::vec2 tangent;
    auto point_on_curve = deCasteljau(u, ctrl_points, tangent);
    points_on_curve.push_back(point_on_curve);

    if (i == num_samples / 2) {
      auto normal = normalize(glm::vec2(-tangent.y, tangent.x));
      branches_p1.push_back(point_on_curve);
      branches_p2.push_back(point_on_curve + normal * length(ray) * 0.5f);

      normal *= -1.f;
      branches_p1.push_back(point_on_curve);
      branches_p2.push_back(point_on_curve + normal * length(ray) * 0.5f);
    }
  }

  for (unsigned int i = 1; i < points_on_curve.size(); i++) {
    cv::line(splat_map, cv_point(points_on_curve[i]), cv_point(points_on_curve[i - 1]), cv::Scalar(0), 2);
  }

  if (length(ray) > splat_map.cols * 0.1f) {
    for (unsigned int i = 0; i < branches_p1.size(); i++) {
      generateSplatMap(branches_p1[i], branches_p2[i], gen, dist, splat_map);
    }
  }
}
