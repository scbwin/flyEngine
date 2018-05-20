#include <opengl/OpenGLAPI.h>
#include <GLWidget.h>
#include <AssimpImporter.h>
#include <btBulletDynamicsCommon.h>
#include <Leakcheck.h>
#include <renderer/Renderer.h>
#include <iostream>
#include <Engine.h>
#include <Transform.h>
#include <GameTimer.h>
#include <Camera.h>
#include <CameraController.h>
#include <AntTweakBar.h>
#include <LevelOfDetail.h>
#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <SkydomeRenderable.h>
#include <AntWrapper.h>
#include <physics/Bullet3PhysicsSystem.h>
#include <physics/RigidBody.h>
#include <random>
#include <StaticInstancedMeshRenderable.h>

GLWidget::GLWidget()
{
  _engine = std::make_unique<fly::Engine>();
  _gameTimer = std::make_unique<fly::GameTimer>();
  setMouseTracking(true);
}

GLWidget::~GLWidget()
{
  TwTerminate();
}

void GLWidget::initializeGL()
{
  _renderer = std::make_shared<fly::Renderer<fly::OpenGLAPI>>(&_graphicsSettings);
  _graphicsSettings.addListener(_renderer);
  _engine->addSystem(_renderer);
#if PHYSICS
  _physicsSystem = std::make_shared<fly::Bullet3PhysicsSystem>();
  _engine->addSystem(_physicsSystem);
#endif
  initGame();
  auto timer = new QTimer(this);
  QObject::connect(timer, &QTimer::timeout, this, static_cast<void(GLWidget::*)()>(&GLWidget::update));
  timer->start(0);
  TwInit(TwGraphAPI::TW_OPENGL_CORE, nullptr);
  _bar = TwNewBar("Stats");
  TwAddButton(_bar, _fpsButtonName, nullptr, nullptr, nullptr);
#if RENDERER_STATS
  TwAddButton(_bar, _renderedMeshesName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedMeshesShadowName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedTrianglesName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _renderedTrianglesShadowName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _bvhTraversalName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _bvhTraversalShadowMapName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _sceneRenderingCPUName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _smRenderingCPUName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _sceneMeshGroupingUName, nullptr, nullptr, nullptr);
  TwAddButton(_bar, _shadowMapGroupingUName, nullptr, nullptr, nullptr);
#endif
  auto settings_bar = TwNewBar("Settings");
  AntWrapper(settings_bar, &_graphicsSettings, _renderer->getApi(), _camController->getCamera().get(), _camController.get(), _skydome.get(), _gameTimer.get(), this);
  TwSetTopBar(_bar);
}

void GLWidget::resizeGL(int width, int height)
{
  _renderer->onResize(fly::Vec2i(width, height));
  TwWindowSize(width, height);
#if PHYSICS
  _viewPortSize = fly::Vec2f(static_cast<float>(width), static_cast<float>(height));
#endif
}

void GLWidget::paintGL()
{
  _gameTimer->tick();
  _renderer->setDefaultRendertarget(defaultFramebufferObject());
#if PHYSICS
  if (_selectedRigidBody) {
    fly::Vec3f focus_pos = _camController->getCamera()->getPosition() + _camController->getCamera()->getDirection() * _focusDist;
    btTransform t;
    _selectedRigidBody->getBtRigidBody()->getMotionState()->getWorldTransform(t);
    fly::Vec3f body_pos(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z());
    if (distance(body_pos, focus_pos) < 1.f) {
      t.setOrigin(btVector3(focus_pos[0], focus_pos[1], focus_pos[2]));
      _selectedRigidBody->getBtRigidBody()->getMotionState()->setWorldTransform(t);
      _selectedRigidBody->getBtRigidBody()->setWorldTransform(t);
      _selectedRigidBody->getBtRigidBody()->setActivationState(ISLAND_SLEEPING);
    }
    else {
      fly::Vec3f impulse = normalize(focus_pos - body_pos) * _impulseStrength * distance(body_pos, focus_pos);
      _selectedRigidBody->getBtRigidBody()->applyImpulse(btVector3(impulse[0], impulse[1], impulse[2]), btVector3(0.f, 0.f, 0.f));
    }
  }
#endif
  if (contains<int>(_keysPressed, 'W')) {
    _camController->stepForward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'A')) {
    _camController->stepLeft(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'S')) {
    _camController->stepBackward(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'D')) {
    _camController->stepRight(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, 'C')) {
    _camController->stepDown(_gameTimer->getDeltaTimeSeconds());
  }
  if (contains<int>(_keysPressed, Qt::Key::Key_Space)) {
    _camController->stepUp(_gameTimer->getDeltaTimeSeconds());
  }
  _engine->update(_gameTimer->getTimeSeconds(), _gameTimer->getDeltaTimeSeconds());
  _fps++;
  if (_gameTimer->getTotalTimeSeconds() >= _measure) {
    _measure = _gameTimer->getTotalTimeSeconds() + 1.f;
    std::string fps_label_str = std::to_string(_fps) + " FPS";
    TwSetParam(_bar, _fpsButtonName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, fps_label_str.c_str());
#if RENDERER_STATS
    const auto& stats = _renderer->getStats();
    TwSetParam(_bar, _renderedMeshesName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Meshes:" + formatNumber(stats._renderedMeshes)).c_str());
    TwSetParam(_bar, _renderedMeshesShadowName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Meshes SM:" + formatNumber(stats._renderedMeshesShadow)).c_str());
    TwSetParam(_bar, _renderedTrianglesName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Triangles:" + formatNumber(stats._renderedTriangles)).c_str());
    TwSetParam(_bar, _renderedTrianglesShadowName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Triangles SM:" + formatNumber(stats._renderedTrianglesShadow)).c_str());
    TwSetParam(_bar, _bvhTraversalName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("BVH traversal microseconds:" + formatNumber(stats._bvhTraversalMicroSeconds)).c_str());
    TwSetParam(_bar, _bvhTraversalShadowMapName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("BVH traversal shadow map microseconds:" + formatNumber(stats._bvhTraversalShadowMapMicroSeconds)).c_str());
    TwSetParam(_bar, _sceneRenderingCPUName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Scene render CPU microseconds:" + formatNumber(stats._sceneRenderingCPUMicroSeconds)).c_str());
    TwSetParam(_bar, _smRenderingCPUName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Shadow map render CPU microseconds:" + formatNumber(stats._shadowMapRenderCPUMicroSeconds)).c_str());
    TwSetParam(_bar, _sceneMeshGroupingUName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Scene mesh grouping microseconds:" + formatNumber(stats._sceneMeshGroupingMicroSeconds)).c_str());
    TwSetParam(_bar, _shadowMapGroupingUName, "label", TwParamValueType::TW_PARAM_CSTRING, 1, ("Shadow map grouping microseconds:" + formatNumber(stats._shadowMapGroupingMicroSeconds)).c_str());
#endif
    _fps = 0;
  }
  TwDraw();
}

void GLWidget::keyPressEvent(QKeyEvent * e)
{
  _keysPressed.insert(e->key());
  if (e->key() == Qt::Key::Key_Shift) {
    _camController->acceleratePressed();
  }
  if (e->key() == Qt::Key::Key_Control) {
    _camController->deceleratePressed();
  }
  if (e->key() == Qt::Key::Key_I) {
  //  _renderer->getApi()->writeShadersToDisk();
  }
}

void GLWidget::keyReleaseEvent(QKeyEvent * e)
{
  _keysPressed.erase(e->key());
  if (e->key() == Qt::Key::Key_Shift) {
    _camController->accelerateReleased();
  }
  if (e->key() == Qt::Key::Key_Control) {
    _camController->decelerateReleased();
  }
}

void GLWidget::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (!TwMouseButton(TwMouseAction::TW_MOUSE_PRESSED, TwMouseButtonID::TW_MOUSE_LEFT)) {
      _camController->mousePress(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
    }
  }
  else if (e->button() == Qt::MouseButton::RightButton || e->button() == Qt::MouseButton::MiddleButton ) {
#if PHYSICS
    _camController->mousePress(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
    _selectedRigidBody = nullptr;
    fly::Vec2f xy_ndc = fly::Vec2f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y())) / _viewPortSize * 2.f - 1.f;
    xy_ndc[1] *= -1.f;
    fly::Vec4f ray_start_ndc(xy_ndc, fly::Vec2f(-1.f, 1.f));
    fly::Vec4f ray_start_world = inverse(_renderer->getViewProjectionMatrix()) * ray_start_ndc;
    ray_start_world /= ray_start_world[3];
    fly::Vec4f ray_end_ndc(xy_ndc, fly::Vec2f(1.f, 1.f));
    fly::Vec4f ray_end_world = inverse(_renderer->getViewProjectionMatrix()) * ray_end_ndc;
    ray_end_world /= ray_end_world[3];
    btCollisionWorld::ClosestRayResultCallback callback = btCollisionWorld::ClosestRayResultCallback(btVector3(ray_start_world[0], ray_start_world[1], ray_start_world[2]),
      btVector3(ray_end_world[0], ray_end_world[1], ray_end_world[2]));
    _physicsSystem->getDynamicsWorld()->rayTest(btVector3(ray_start_world[0], ray_start_world[1], ray_start_world[2]),
      btVector3(ray_end_world[0], ray_end_world[1], ray_end_world[2]), callback);
    if (callback.hasHit()) {
      fly::RigidBody* rigid_body = reinterpret_cast<fly::RigidBody*>(callback.m_collisionObject->getUserPointer());
      if (rigid_body) {
        rigid_body->getBtRigidBody()->activate();
        _selectedRigidBody = rigid_body;
        _smashItem = e->button() == Qt::MouseButton::RightButton;
      }
    }
#endif
  }
}

void GLWidget::mouseMoveEvent(QMouseEvent * e)
{
  if (_camController->isPressed()) {
    _camController->mouseMove(fly::Vec3f(static_cast<float>(e->localPos().x()), static_cast<float>(e->localPos().y()), 0.f));
  }
  else {
    TwMouseMotion(e->localPos().x(), e->localPos().y());
  }
}

void GLWidget::mouseReleaseEvent(QMouseEvent * e)
{
  if (e->button() == Qt::MouseButton::LeftButton) {
    if (_camController->isPressed()) {
      _camController->mouseRelease();
    }
    else {
      _camController->mouseRelease();
      TwMouseButton(TwMouseAction::TW_MOUSE_RELEASED, TwMouseButtonID::TW_MOUSE_LEFT);
    }
  }
  if (e->button() == Qt::MouseButton::RightButton || e->button() == Qt::MouseButton::MiddleButton) {
#if PHYSICS
    if (_selectedRigidBody) {
      _selectedRigidBody->getBtRigidBody()->activate();
      if (_smashItem) {
        fly::Vec3f impulse = _camController->getCamera()->getDirection() * _impulseStrengthSmash;
        _selectedRigidBody->getBtRigidBody()->applyImpulse(btVector3(impulse[0], impulse[1], impulse[2]), btVector3(0, 0, 0));
      }
    }
    _selectedRigidBody = nullptr;

    if (_camController->isPressed()) {
      _camController->mouseRelease();
    }
    else {
      _camController->mouseRelease();
      TwMouseButton(TwMouseAction::TW_MOUSE_RELEASED, TwMouseButtonID::TW_MOUSE_RIGHT);
    }
#endif
  }
}

void GLWidget::wheelEvent(QWheelEvent * e)
{
}

void GLWidget::initGame()
{
  fly::Timing init_game_timing;
  auto importer = std::make_shared<fly::AssimpImporter>();
#if TREE_SCENE
  auto tree_model = importer->loadModel("assets/tree.obj");
  tree_model->mergeMeshesByMaterial();
  for (unsigned i = 0; i < 100; i++) {
    for (unsigned j = 0; j < 100; j++) {
      for (const auto& m : tree_model->getMeshes()) {
        auto entity = _engine->getEntityManager()->createEntity();
        entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m, tree_model->getMaterials()[m->getMaterialIndex()], fly::Transform(fly::Vec3f(i * 5.f, 0.f, j * 5.f), fly::Vec3f(0.01f)).getModelMatrix(), false));
      }
    }
  }
#else
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  sponza_model->getMaterials()[10]->setIsReflective(true);
 // for (const auto& m : sponza_model->getMaterials()) {
    //m->setIsReflective(true);
  //}
#if DELETE_CURTAIN
  auto meshes = sponza_model->getMeshes();
  meshes.erase(meshes.end() - 28);
  sponza_model->setMeshes(meshes);
#endif
  for (const auto& m : sponza_model->getMaterials()) {
    m->setSpecularExponent(32.f);
    if (m->getDiffusePath() == "assets/sponza/textures\\spnza_bricks_a_diff.tga") {
      m->setHeightPath("assets/DisplacementMap.png");
      m->setParallaxHeightScale(0.05f);
    }
    else if (m->getDiffusePath() == "assets/sponza/textures\\sponza_floor_a_diff.tga") {
      m->setHeightPath("assets/height.png");
      m->setParallaxHeightScale(0.021f);
    }
  }

  fly::Vec3f sponza_scale(0.01f);
#if PHYSICS
  std::vector<std::shared_ptr<btCollisionShape>> _sponzaShapes;
  for (const auto& mesh : sponza_model->getMeshes()) {
    _triangleMeshes.push_back(std::make_shared<btTriangleMesh>());
    for (unsigned i = 0; i < mesh->getIndices().size(); i += 3) {
      auto v0 = mesh->getVertices()[mesh->getIndices()[i]]._position;
      auto v1 = mesh->getVertices()[mesh->getIndices()[i + 1]]._position;
      auto v2 = mesh->getVertices()[mesh->getIndices()[i + 2]]._position;
      _triangleMeshes.back()->addTriangle(btVector3(v0[0], v0[1], v0[2]), btVector3(v1[0], v1[1], v1[2]), btVector3(v2[0], v2[1], v2[2]));
    }
    _sponzaShapes.push_back(std::make_shared<btBvhTriangleMeshShape>(_triangleMeshes.back().get(), true, true));
    _sponzaShapes.back()->setLocalScaling(btVector3(sponza_scale[0], sponza_scale[1], sponza_scale[2]));
  }
#endif

#if TOWERS
  std::mt19937 gen;
  std::uniform_real_distribution<float> scale_dist(50.f, 250.f);
  std::vector<std::shared_ptr<fly::StaticMeshRenderable>> towers;
  auto tower_model = importer->loadModel("assets/cube.obj");
  for (int x = 0; x < NUM_TOWERS; x++) {
    for (int y = 0; y < NUM_TOWERS; y++) {
      auto tower = _engine->getEntityManager()->createEntity();
      float scale = scale_dist(gen);
      tower->addComponent(std::make_shared<fly::StaticMeshRenderable>(tower_model->getMeshes().front(), tower_model->getMeshes().front()->getMaterial(), fly::Transform(fly::Vec3f(x * 350.f, scale, y * 350.f), fly::Vec3f(scale / 3.f, scale, scale / 3.f)).getModelMatrix(), false));
      towers.push_back(tower->getComponent<fly::StaticMeshRenderable>());
    }
  }
#endif

#if SPONZA_MANY
  for (int x = 0; x < NUM_OBJECTS; x++) {
    for (int y = 0; y < NUM_OBJECTS; y++) {
      auto model_matrix = fly::Transform(fly::Vec3f(x * 60.f, -sponza_model->getAABB()->getMin()[1] * sponza_scale[1], y * 60.f), fly::Vec3f(sponza_scale)).getModelMatrix();
#else
  auto model_matrix = fly::Transform(fly::Vec3f(0.f), sponza_scale).getModelMatrix();
#endif
#if TOWERS && SPONZA_MANY
  fly::AABB sponza_aabb_world(*sponza_model->getAABB(), model_matrix);
  bool intersects = false;
  for (const auto& t : towers) {
    if (t->getAABBWorld()->intersects(sponza_aabb_world)) {
      intersects = true;
    }
  }
#endif
#if TOWERS && SPONZA_MANY
  if (!intersects) {
#endif
    unsigned index = 0;
#if SPONZA
    for (const auto& mesh : sponza_model->getMeshes()) {
      auto entity = _engine->getEntityManager()->createEntity();
      bool has_wind = index >= 44 && index <= 61;
      if (has_wind) {
       // std::cout << mesh->getMaterial()->getDiffusePath() << " " << index << std::endl;
      }
      fly::Vec3f aabb_offset = has_wind ? fly::Vec3f(0.f, 0.f, 0.25f) : fly::Vec3f(0.f);
      //fly::Vec3f translation(0.f);
#if !DELETE_CURTAIN
      if (index == sponza_model->getMeshes().size() - 28) {
        has_wind = true;
        aabb_offset = fly::Vec3f(0.f, 0.f, 0.25f);
        mesh->setMaterial(sponza_model->getMeshes()[44]->getMaterial());
        //  translation[1] = 1.f;
      }
#endif
      fly::AABB aabb_world(*mesh->getAABB(), model_matrix);
      aabb_world.expand(aabb_offset);
      entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(mesh,
#if SPONZA_MANY
        mesh->getMaterial(), model_matrix, has_wind, aabb_world));
#else
        mesh->getMaterial(), model_matrix, has_wind, aabb_offset));
#endif
#if PHYSICS
      const auto& model_matrix = entity->getComponent<fly::StaticMeshRenderable>()->getModelMatrix();
      entity->addComponent(std::make_shared<fly::RigidBody>(model_matrix[3].xyz(), 0.f, _sponzaShapes[index], 0.1f));
#endif
      index++;
    }
#endif
#if TOWERS
  }
#endif
#if SPONZA_MANY
    }
  }
#endif
#endif
#if PHYSICS || SKYDOME
  auto sphere_model = importer->loadModel("assets/sphere.obj");
  for (const auto& m : sphere_model->getMaterials()) {
    m->setSpecularExponent(256.f);
    m->setDiffuseColor(fly::Vec3f(1.f, 0.f, 0.f));
  }
#endif
#if SKYDOME
  _skydome = _engine->getEntityManager()->createEntity();
  _skydome->addComponent(sphere_model);
  _skydome->addComponent(std::make_shared<fly::SkydomeRenderable>(sphere_model->getMeshes().front()));
#endif
#if PHYSICS
 // _graphicsSettings.setDebugObjectAABBs(true);
  std::mt19937 gen;
  std::uniform_real_distribution<float> col_dist(0.f, 2.f);
  std::uniform_real_distribution<float> scale_dist(0.3f, 0.55f);
  for (const auto& m : sphere_model->getMeshes()) {
    auto vec = m->getAABB()->getMax() - m->getAABB()->getMin();
    float radius = std::max(vec[0], std::max(vec[1], vec[2])) * 0.5f;
    for (unsigned i = 0; i <= 30; i++) {
      auto col_shape = std::make_shared<btSphereShape>(radius);
      float scale = scale_dist(gen);
      col_shape->setLocalScaling(btVector3(scale, scale, scale));
      float mass = 0.02f;
      float restitution = 1.f;
      auto mat = std::make_shared<fly::Material>();
      mat->setIsReflective(true);
      mat->setNormalPath("assets/ground_normals.png");
      mat->setDiffuseColor(fly::Vec3f(col_dist(gen), col_dist(gen), col_dist(gen)));
      auto ent = _engine->getEntityManager()->createEntity();
      ent->addComponent(std::make_shared<fly::RigidBody>(fly::Vec3f(0.f, 30.f + i * 2.f, 0.f), mass, col_shape, restitution));
      ent->addComponent(std::make_shared<fly::DynamicMeshRenderable>(m, mat, ent->getComponent<fly::RigidBody>()));
      _rigidBodys.push_back(ent->getComponent<fly::RigidBody>());
      _rigidBodys.back()->getBtRigidBody()->setUserPointer(ent->getComponent<fly::RigidBody>().get());
    }
  }
  auto cube_model = importer->loadModel("assets/cube.obj");
  for (const auto& m : cube_model->getMeshes()) {
    auto half_extents = (m->getAABB()->getMax() - m->getAABB()->getMin()) * 0.5f;
    for (unsigned i = 0; i < 50; i++) {
      auto cube_shape = std::make_shared<btBoxShape>(btVector3(half_extents[0], half_extents[1], half_extents[2]));
      float scale = scale_dist(gen);
      cube_shape->setLocalScaling(btVector3(scale, scale, scale));
      float mass = 0.02f;
      float restitution = 1.f;
      auto ent = _engine->getEntityManager()->createEntity();
      ent->addComponent(std::make_shared<fly::RigidBody>(fly::Vec3f(3.f, 25 + i * 2.f, 0.f), mass, cube_shape, restitution));
      auto material = std::make_shared<fly::Material>();
      material->setDiffuseColor(fly::Vec3f(col_dist(gen), col_dist(gen), col_dist(gen)));
      material->setIsReflective(true);
      material->setNormalPath("assets/ground_normals.png");
      ent->addComponent(std::make_shared<fly::DynamicMeshRenderable>(m, material, ent->getComponent<fly::RigidBody>()));
      _rigidBodys.push_back(ent->getComponent<fly::RigidBody>());
      _rigidBodys.back()->getBtRigidBody()->setUserPointer(ent->getComponent<fly::RigidBody>().get());
    }
  }
#endif

#if SPONZA_MANY || TREE_SCENE
  auto plane_model = importer->loadModel("assets/plane.obj");
  for (const auto& m : plane_model->getMaterials()) {
    m->setNormalPath("assets/ground_normals.png");
    m->setDiffuseColor(fly::Vec3f(0.870f, 0.768f, 0.329f) * 1.5f);
  }
  for (const auto& m : plane_model->getMeshes()) {
    std::vector<fly::Vertex> vertices_new;
    for (const auto& v : m->getVertices()) {
      fly::Vertex v_new = v;
      v_new._uv *= (_renderer->getSceneMax().xz() - _renderer->getSceneMin().xz()) * 0.65f;
      vertices_new.push_back(v_new);
    }
    m->setVertices(vertices_new);
  }
  for (const auto& m : plane_model->getMeshes()) {
    auto entity = _engine->getEntityManager()->createEntity();
    auto scale = _renderer->getSceneMax() - _renderer->getSceneMin();
    scale[1] = 1.f;
    auto translation = _renderer->getSceneMin();
    entity->addComponent(std::make_shared<fly::StaticMeshRenderable>(m,
      plane_model->getMaterials()[m->getMaterialIndex()], fly::Transform(translation, scale).getModelMatrix(), false));
  }
#endif

#if INSTANCED_MESHES
  fly::Vec2i num_meshes(NUM_INSTANCED_MESHES_PER_DIR);
  auto instanced_entity = _engine->getEntityManager()->createEntity();
  std::vector<fly::Mat4f> model_matrices;
  for (int x = 0; x < num_meshes[0]; x++) {
    for (int z = 0; z < num_meshes[1]; z++) {
      model_matrices.push_back(fly::Transform(fly::Vec3f(static_cast<float>(x), 0.f, static_cast<float>(z)) * 5.f).getModelMatrix());
    }
  }
  auto mesh = sphere_model->getMeshes()[0];
  auto instanced_renderable = std::make_shared<fly::StaticInstancedMeshRenderable>(mesh, mesh->getMaterial(), *mesh->getAABB(), model_matrices);
  instanced_entity->addComponent(instanced_renderable);
#endif

  auto cam_entity = _engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(4.f, 2.f, 0.f), glm::vec3(glm::radians(270.f), 0.f, 0.f)));
  auto dl_entity = _engine->getEntityManager()->createEntity();
  _dl = std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), glm::vec3(-1000.f, 2000.f, -1000.f), glm::vec3(-500.f, 0.f, -500.f));
  dl_entity->addComponent(_dl);

  _camController = std::make_unique<fly::CameraController>(cam_entity->getComponent<fly::Camera>(), 20.f);
#if SPONZA_MANY
  _camController->setSpeed(100.f);
#endif
  std::cout << "Init game took " << init_game_timing.duration<std::chrono::milliseconds>() << " milliseconds." << std::endl;
}

std::string GLWidget::formatNumber(unsigned number)
{
  unsigned num = number;
  std::string ret;
  while (true) {
    unsigned remainder = num % 1000;
    num /= 1000;
    char rem_str[4];
    if (num == 0) {
      if (remainder >= 100) {
        sprintf_s(rem_str, "%3u", remainder);
      }
      else if (remainder >= 10) {
        sprintf_s(rem_str, "%2u", remainder);
      }
      else {
        sprintf_s(rem_str, "%1u", remainder);
      }
    }
    else {
      sprintf_s(rem_str, "%03u", remainder);
    }
    ret = rem_str + ret;
    if (num == 0) {
      break;
    }
    else {
      ret = "," + ret;
    }
  }
  return ret;
}
