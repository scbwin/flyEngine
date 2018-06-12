//#include <opengl/OpenGLAPI.h>
#include <AssimpImporter.h>
#include <GLWidget.h>
#include <iostream>
#include <opengl/OpenGLUtils.h>
#include <math/MathHelpers.h>
//#include <renderer/Renderer.h>
#include <Model.h>
#include <Mesh.h>
#include <Material.h>
#include <renderer/MeshRenderables.h>
#include <PhysicsCameraController.h>
#include <CameraController.h>
#include <Light.h>
#include <qopenglfunctions_4_5_core.h>
#include <SOIL/SOIL.h>

GLWidget::GLWidget() :
  _camera(std::make_shared<fly::Camera>(fly::Vec3f(0.f, 2.f, 0.f), fly::Vec3f(0.f, 0.f, 0.f))),
  _dl(std::make_shared<fly::DirectionalLight>(fly::Vec3f(2.f), fly::Vec3f(0.5f, -0.58f, 0.5f))),
  _physicsCC(std::make_shared<fly::PhysicsCameraController>(_camera)),
  _cc(std::make_shared<fly::CameraController>(_camera, 20.f))
{
  _pp._near = 0.1f;
  _pp._far = 10000.f;
  _pp._fieldOfViewDegrees = 45.f;
  setMouseTracking(true);
}

GLWidget::~GLWidget()
{
  TwTerminate();
}

void GLWidget::initializeGL()
{
  glewExperimental = true;
  glewInit();
  _gs = std::make_shared<fly::GraphicsSettings>();
  _engine.addSystem(_physicsCC);
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
  initializeOpenGLFunctions();
  GL_CHECK(glClearColor(0.149f, 0.509f, 0.929f, 1.f));
  fly::GLShaderSource vs("assets/opengl/vs_terrain_edit.glsl", GL_VERTEX_SHADER);
  fly::GLShaderSource tcs("assets/opengl/tcs_terrain_edit.glsl", GL_TESS_CONTROL_SHADER);
  fly::GLShaderSource tce("assets/opengl/tce_terrain_edit.glsl", GL_TESS_EVALUATION_SHADER);
  fly::GLShaderSource fs("assets/opengl/fs_terrain_edit.glsl", GL_FRAGMENT_SHADER);
  _terrainShader = std::make_unique<fly::GLShaderProgram>();
  _terrainShader->add(vs);
  _terrainShader->add(tcs);
  _terrainShader->add(tce);
  _terrainShader->add(fs);
  _terrainShader->link();

  _terrainShaderWireframe = std::make_unique<fly::GLShaderProgram>();
  _terrainShaderWireframe->add(vs);
  _terrainShaderWireframe->add(tcs);
  _terrainShaderWireframe->add(tce);
  _terrainShaderWireframe->add(fly::GLShaderSource("assets/opengl/fs_terrain_wireframe.glsl", GL_FRAGMENT_SHADER));
  _terrainShaderWireframe->link();

  TwInit(TwGraphAPI::TW_OPENGL, nullptr);
  auto bar = TwNewBar("Settings");
  TwAddVarRW(bar, "Patches per dir", TwType::TW_TYPE_INT32, &_patchesPerDir, nullptr);
  TwAddVarRW(bar, "Patch size", TwType::TW_TYPE_INT32, &_patchSize, nullptr);
  TwAddVarRW(bar, "Octaves", TwType::TW_TYPE_UINT32, &_octaves, nullptr);
  TwAddVarRW(bar, "Max tess distance", TwType::TW_TYPE_FLOAT, &_maxTessDistance, nullptr);
  TwAddVarRW(bar, "Wireframe", TwType::TW_TYPE_BOOLCPP, &_wireFrame, nullptr);
  TwAddVarRW(bar, "Amplitude", TwType::TW_TYPE_FLOAT, &_amplitude, "step = 0.01f");
  TwAddVarRW(bar, "Frequency", TwType::TW_TYPE_FLOAT, &_frequency, "step = 0.0001f");
  TwAddVarRW(bar, "Seed 0", TwType::TW_TYPE_FLOAT, &_seed[0], "step = 0.01f");
  TwAddVarRW(bar, "Seed 1", TwType::TW_TYPE_FLOAT, &_seed[1], "step = 0.01f");
  TwAddVarRW(bar, "Seed 2", TwType::TW_TYPE_FLOAT, &_seed[2], "step = 0.01f");
  TwAddVarRW(bar, "Terraces", TwType::TW_TYPE_BOOLCPP, &_terraces, nullptr);
  TwAddVarRW(bar, "Terrain color", TwType::TW_TYPE_COLOR3F, &_terrainColor, nullptr);
  TwAddVarRW(bar, "Exponent", TwType::TW_TYPE_FLOAT, &_exponent, "step = 0.001f");
  TwAddVarRW(bar, "Persistence", TwType::TW_TYPE_FLOAT, &_persistence, "step = 0.001f");
  TwAddVarRW(bar, "Lacunarity", TwType::TW_TYPE_FLOAT, &_lacunarity, "step = 0.001f");
  TwAddVarCB(bar, "Light dir x", TwType::TW_TYPE_FLOAT, setLightDirX, getLightDirX, _dl.get(), "step = 0.001");
  TwAddVarCB(bar, "Light dir y", TwType::TW_TYPE_FLOAT, setLightDirY, getLightDirY, _dl.get(), "step = 0.001");
  TwAddVarCB(bar, "Light dir z", TwType::TW_TYPE_FLOAT, setLightDirZ, getLightDirZ, _dl.get(), "step = 0.001");
  TwAddVarRW(bar, "Snow level low", TwType::TW_TYPE_FLOAT, &_snowLevel[0], "step = 0.01f");
  TwAddVarRW(bar, "Snow level high", TwType::TW_TYPE_FLOAT, &_snowLevel[1], "step = 0.01f");
  TwAddVarRW(bar, "Ground level low", TwType::TW_TYPE_FLOAT, &_groundLevel[0], "step = 0.01f");
  TwAddVarRW(bar, "Ground level high", TwType::TW_TYPE_FLOAT, &_groundLevel[1], "step = 0.01f");
  TwAddVarRW(bar, "Ground color", TwType::TW_TYPE_COLOR3F, &_groundColor[0], nullptr);
  TwAddVarRW(bar, "Ground color2", TwType::TW_TYPE_COLOR3F, &_groundColor2[0], nullptr);
  TwAddVarRW(bar, "Snow intensity", TwType::TW_TYPE_FLOAT, &_snowIntensity, "step = 0.01f");
  TwAddVarRW(bar, "Steepness exponent", TwType::TW_TYPE_FLOAT, &_steepnessExponent, "step = 0.01f");
  TwAddVarRW(bar, "Steepness exponent low", TwType::TW_TYPE_FLOAT, &_steepnessExponentLow, "step = 0.01f");
  TwAddVarRW(bar, "Ambient", TwType::TW_TYPE_FLOAT, &_ambient, "step = 0.001f");
  TwAddVarRW(bar, "Fog distance", TwType::TW_TYPE_FLOAT, &_fogDistance, "step = 1.f");
  TwAddVarRW(bar, "Fog color", TwType::TW_TYPE_COLOR3F, &_fogColor[0], nullptr);

  _skydomeShader = std::make_unique<fly::GLShaderProgram>();
  _skydomeShader->add(fly::GLShaderSource("assets/opengl/vs_skybox.glsl", GL_VERTEX_SHADER));
  _skydomeShader->add(fly::GLShaderSource("assets/opengl/fs_skydome_new.glsl", GL_FRAGMENT_SHADER));
  _skydomeShader->link();

  _skydomeVao = std::make_unique<fly::GLVertexArray>();
  auto sphere_mesh = fly::AssimpImporter().loadModel("assets/sphere.obj")->getMeshes().front();
  _skydomeVbo = std::make_unique<fly::GLBuffer>(GL_ARRAY_BUFFER);
  _skydomeVbo->setData(sphere_mesh->getVertices().data(), sphere_mesh->getVertices().size());
  _skydomeIbo = std::make_unique<fly::GLBuffer>(GL_ELEMENT_ARRAY_BUFFER);
  _skydomeIbo->setData(sphere_mesh->getIndices().data(), sphere_mesh->getIndices().size());
  _skydomeVao->bind();
  _skydomeVbo->bind();
  _skydomeIbo->bind();
  GL_CHECK(glEnableVertexAttribArray(0));
  GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(fly::Vertex), reinterpret_cast<const void*>(offsetof(fly::Vertex, _position))));
  _skydomeIndices = static_cast<unsigned>(sphere_mesh->getIndices().size());
}

void GLWidget::resizeGL(int width, int height)
{
  //_renderer->onResize(fly::Vec2i(width, height));
  _projectionMatrix = fly::MathHelpers::getProjectionMatrixPerspective(_pp._fieldOfViewDegrees, static_cast<float>(width) / static_cast<float>(height), 
    _pp._near, _pp._far, fly::ZNearMapping::MINUS_ONE);
  TwWindowSize(width, height);
}

void GLWidget::paintGL()
{
  //_renderer->setDefaultRendertarget(defaultFramebufferObject());
  GL_CHECK(glEnable(GL_DEPTH_TEST));
  GL_CHECK(glDepthFunc(GL_LEQUAL));
  _camera->updateViewMatrix();
  auto vp = _projectionMatrix * _camera->getViewMatrix();
  GL_CHECK(glPatchParameteri(GL_PATCH_VERTICES, 4));
  GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  GL_CHECK(glViewport(0, 0, width(), height()));
  GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, _wireFrame ? GL_LINE : GL_FILL));
  auto& shader = _wireFrame ? _terrainShaderWireframe : _terrainShader;
  shader->bind();
  float terrain_size_half = _patchesPerDir * _patchSize * 0.5f;
  fly::setMatrix(shader->uniformLocation("view_projection_matrix"), vp);
  fly::setScalar(shader->uniformLocation("amplitude"), _amplitude);
  fly::setScalar(shader->uniformLocation("frequency"), _frequency);
  fly::setScalar(shader->uniformLocation("patches_per_dir"), _patchesPerDir);
  fly::setScalar(shader->uniformLocation("patch_size"), static_cast<float>(_patchSize));
  fly::setScalar(shader->uniformLocation("terrain_size_half"), terrain_size_half);
  fly::setVector(shader->uniformLocation("cam_pos_world"), _camera->getPosition());
  fly::setVector(shader->uniformLocation("seed"), _seed);
  fly::setScalar(shader->uniformLocation("terraces"), _terraces);
  fly::setScalar(shader->uniformLocation("exponent"), _exponent);
  fly::setScalar(shader->uniformLocation("persistence"), _persistence);
  fly::setScalar(shader->uniformLocation("lacunarity"), _lacunarity);
  fly::setScalar(shader->uniformLocation("octaves"), _octaves);
  fly::setScalar(shader->uniformLocation("max_distance"), _maxTessDistance);
  if (!_wireFrame) {
    fly::setVector(shader->uniformLocation("snow_level"), _snowLevel);
    fly::setScalar(shader->uniformLocation("steepness_exponent"), _steepnessExponent);
    fly::setScalar(shader->uniformLocation("steepness_exponent_low"), _steepnessExponentLow);
    fly::setScalar(shader->uniformLocation("snow_intensity"), _snowIntensity);
    fly::setVector(shader->uniformLocation("ground_color"), _groundColor);
    fly::setVector(shader->uniformLocation("ground_color2"), _groundColor2);
    fly::setVector(shader->uniformLocation("ground_level"), _groundLevel);
    fly::setVector(shader->uniformLocation("terrain_color"), _terrainColor);
    fly::setScalar(shader->uniformLocation("ambient"), _ambient);
    fly::setVector(shader->uniformLocation("light_dir_inv"), _dl->getDirection() * -1.f);
    fly::setVector(shader->uniformLocation("view_matrix_third_row"), _camera->getViewMatrix().row(2));
    fly::setVector(shader->uniformLocation("fog_color"), _fogColor);
    fly::setScalar(shader->uniformLocation("fog_distance"), _fogDistance);
    fly::setVector(shader->uniformLocation("light_intensity"), _dl->getIntensity());
  }
  GL_CHECK(glDrawArrays(GL_PATCHES, 0, 4 * _patchesPerDir * _patchesPerDir));

  _skydomeShader->bind();
  fly::setVector(_skydomeShader->uniformLocation("seed"), _seed);
  fly::setScalar(_skydomeShader->uniformLocation("exponent"), _exponentSky);
  fly::setScalar(_skydomeShader->uniformLocation("persistence"), _persistenceSky);
  fly::setScalar(_skydomeShader->uniformLocation("lacunarity"), _lacunaritySky);
  fly::setScalar(_skydomeShader->uniformLocation("amplitude"), _amplitudeSky);
  fly::setScalar(_skydomeShader->uniformLocation("frequency"), _frequencySky);
  fly::setScalar(_skydomeShader->uniformLocation("time"), _engine.getGameTimer()->getTimeSeconds() * 0.01f);
  _skydomeVao->bind();
  _skydomeVbo->bind();
  _skydomeIbo->bind();
  fly::Mat4f view_matrix_sky_dome = _camera->getViewMatrix();
  view_matrix_sky_dome[3] = fly::Vec4f(fly::Vec3f(0.f), 1.f);
  auto skydome_vp = _projectionMatrix * view_matrix_sky_dome;
  fly::setMatrix(_skydomeShader->uniformLocation("VP"), skydome_vp);
  GL_CHECK(glDrawElements(GL_TRIANGLES, _skydomeIndices, GL_UNSIGNED_INT, nullptr));

  _engine.update();

  TwDraw();

  fly::Vec3f acc_dir(0.f);
  if (contains<int>(_keysPressed, 'W')) {
    acc_dir += _camera->getDirection();
  }
  if (contains<int>(_keysPressed, 'A')) {
    acc_dir -= _camera->getRight();
  }
  if (contains<int>(_keysPressed, 'S')) {
    acc_dir -= _camera->getDirection();
  }
  if (contains<int>(_keysPressed, 'D')) {
    acc_dir += _camera->getRight();
  }
  if (contains<int>(_keysPressed, 'C')) {
    acc_dir -= _camera->getUp();
  }
  if (contains<int>(_keysPressed, Qt::Key::Key_Space)) {
    acc_dir += _camera->getUp();
  }
  float acc;
  if (contains<int>(_keysPressed, Qt::Key::Key_Shift)) {
    acc = _camAccelerationHigh;
  }
  else {
    acc = contains<int>(_keysPressed, Qt::Key::Key_Control) ? _camAccelerationLow : _camAccelerationDefault;
  }
  _physicsCC->setAcceleration(acc_dir, acc);
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
}

void GLWidget::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
  if (!TwMouseButton(TwMouseAction::TW_MOUSE_PRESSED, TwMouseButtonID::TW_MOUSE_LEFT)) {
    _cc->mousePress(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (_cc->isPressed()) {
    _cc->mouseMove(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
  }
  else {
    TwMouseMotion(e->localPos().x(), e->localPos().y());
  }
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (!TwMouseButton(TwMouseAction::TW_MOUSE_RELEASED, TwMouseButtonID::TW_MOUSE_LEFT)) {
    _cc->mouseRelease();
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}

void GLWidget::setLightDirX(const void * value, void * client_data)
{
  auto direction = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection();
  direction[0] = *reinterpret_cast<const float*>(value);
  reinterpret_cast<fly::DirectionalLight*>(client_data)->setDirection(direction);
}

void GLWidget::getLightDirX(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection()[0];
}
void GLWidget::setLightDirY(const void * value, void * client_data)
{
  auto direction = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection();
  direction[1] = *reinterpret_cast<const float*>(value);
  reinterpret_cast<fly::DirectionalLight*>(client_data)->setDirection(direction);
}

void GLWidget::getLightDirY(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection()[1];
}
void GLWidget::setLightDirZ(const void * value, void * client_data)
{
  auto direction = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection();
  direction[2] = *reinterpret_cast<const float*>(value);
  reinterpret_cast<fly::DirectionalLight*>(client_data)->setDirection(direction);
}

void GLWidget::getLightDirZ(void * value, void * client_data)
{
  *reinterpret_cast<float*>(value) = reinterpret_cast<fly::DirectionalLight*>(client_data)->getDirection()[2];
}
