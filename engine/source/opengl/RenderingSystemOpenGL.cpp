#define GLM_ENABLE_EXPERIMENTAL
#include <opencv2/opencv.hpp>
#include "opengl/RenderingSystemOpenGL.h"
#include <iostream>
#include "opengl/OpenGLUtils.h"
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <Model.h>
#include "Entity.h"
#include "SOIL/SOIL.h"
#include <fstream>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Camera.h>
#include <Light.h>
#include <Transform.h>
#include <Material.h>
#include <Billboard.h>
#include <Terrain.h>

namespace fly
{
  RenderingSystemOpenGL::RenderingSystemOpenGL()
  {
  }
  void RenderingSystemOpenGL::init(const Vec2i& window_size)
  {
    glewExperimental = true;
    auto result = glewInit();
    if (result != GLEW_OK) {
      std::cout << glewGetErrorString(result) << std::endl;
    }
    else {
      //  std::cout << "Initialized GLEW" << std::endl;
    }

    GLint major_version, minor_version;
    GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, &major_version));
    GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, &minor_version));

    std::cout << "OpenGL version:" << major_version << "." << minor_version << std::endl;

    initShaders();

    _skyboxVertexArray = std::make_shared<GLVertexArrayOld>();
    _skyboxVertexArray->create();
    _skyboxVertexArray->bind();
    GL_CHECK(glEnableVertexAttribArray(0));
    std::array<glm::vec3, 8> skybox_vertices = {
      glm::vec3(-1, -1, -1),
      glm::vec3(1, -1, -1),
      glm::vec3(-1, 1, -1),
      glm::vec3(-1, -1, 1),
      glm::vec3(1, 1, -1),
      glm::vec3(1, -1, 1),
      glm::vec3(-1, 1, 1),
      glm::vec3(1, 1, 1) };

    _skyboxVertexbuffer = std::make_shared<GLBufferOld>();
    _skyboxVertexbuffer->create(GL_ARRAY_BUFFER);
    _skyboxVertexbuffer->bind();
    _skyboxVertexbuffer->setData(&skybox_vertices[0], skybox_vertices.size() * sizeof(skybox_vertices[0]));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0));

    _skyBoxIndices = {
      1, 5, 4, 4, 5, 7, // right
      0, 2, 3, 2, 6, 3, // left
      2, 4, 6, 4, 7, 6, // top
      0, 3, 1, 1, 3, 5, // bottom
      3, 6, 5, 5, 6, 7, // front
      0, 1, 2, 1, 4, 2 // back
    };

    _skyboxIndexbuffer = std::make_shared<GLBufferOld>();
    _skyboxIndexbuffer->create(GL_ELEMENT_ARRAY_BUFFER);
    _skyboxIndexbuffer->bind();
    _skyboxIndexbuffer->setData(&_skyBoxIndices.front(), _skyBoxIndices.size() * sizeof(_skyBoxIndices.front()));

    _grassQuadTree = std::make_shared<GrassQuadTree>();

    int size = _grassQuadTree->getSize();
    cv::Mat mat(size, size, CV_8UC1, cv::Scalar(0));
    for (auto& n : _grassQuadTree->getNodes()) {
      cv::Rect rect(n->_pos.x, n->_pos.y, n->_size, n->_size);
      cv::rectangle(mat, rect, CV_RGB(255, 255, 255));
    }
    cv::imwrite("assets/grass_quadtree_debug.png", mat);

    onResize(window_size);
  }

  void RenderingSystemOpenGL::update(float time, float delta_time)
  {
    if (_viewportSize == glm::ivec2(0)) {
      return;
    }
    prepareRender(delta_time, time);
    renderImpostors();
    if (!_renderWireFrame) {
      renderShadowMaps();
      geometryPass();
      lightingPass();
      renderWater();
      postProcessing(delta_time);
    }
    else {
      renderWireFrame();
    }
    renderToScreen();
    swapParams();
    processTextureFutures();
  }

  bool RenderingSystemOpenGL::renderGodRaysScreenSpace(const glm::vec3 & light_pos)
  {
    glm::vec3 light_pos_screen = toScreenSpace(glm::mat4(), light_pos, _viewMatrix, _projectionMatrix);
    glm::vec3 light_pos_view_space = glm::mat4(_viewMatrix) * glm::vec4(light_pos, 1.f);
    bool light_visible = light_pos_view_space.z < 0.f && glm::all(glm::greaterThan(light_pos_screen, glm::vec3(0.f))) && glm::all(glm::lessThan(glm::vec2(light_pos_screen), glm::vec2(1.f)));
    if (!light_visible) {
      return false;
    }
    float dist_to_border = std::min(light_pos_screen.x, std::min(1.f - light_pos_screen.x, std::min(light_pos_screen.y, 1.f - light_pos_screen.y)));
    float fade_dist = 0.1f;
    float fade = glm::clamp(dist_to_border / fade_dist, 0.f, 1.f);

    _framebufferQuarterSize[0]->bind();
    _framebufferQuarterSize[0]->setViewport();
    auto shader = _shaderProgramGodRay;
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    _gBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 0));
    GL_CHECK(glUniform1f(shader->uniformLocation("numSamples"), _godRaySamples));
    GL_CHECK(glUniform1f(shader->uniformLocation("fade"), fade));
    GL_CHECK(glUniform1f(shader->uniformLocation("decay"), _godRayDecay));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosScreen"), light_pos_screen.x, light_pos_screen.y, light_pos_screen.z));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    return true;
  }

  void RenderingSystemOpenGL::renderAABBs()
  {
    /* _frontBuffer->bind();
     _frontBuffer->setViewport();

     GL_CHECK(glEnable(GL_DEPTH_TEST));
     _frontBuffer->setDepthTexture(_sceneDepthbuffer);

     auto shader = _aabbShader;
     shader->bind();

     auto vertex_array = std::make_shared<GLVertexArray>();
     vertex_array->create();
     vertex_array->bind();
     GL_CHECK(glEnableVertexAttribArray(0));

     auto vertex_buffer = std::make_shared<GLBuffer>();
     vertex_buffer->create(GL_ARRAY_BUFFER);
     vertex_buffer->bind();

     auto index_buffer = std::make_shared<GLBuffer>();
     index_buffer->create(GL_ELEMENT_ARRAY_BUFFER);
     index_buffer->bind();

     for (auto& m : _models) {
       auto model = m->getComponent<Model>();
       auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
       auto mvp = _projectionMatrix * _viewMatrix * model_matrix;
       GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
       auto indices = model->getMeshes()[0]->getAABBLineIndices();
       index_buffer->setData(&indices[0], indices.size() * sizeof(indices[0]));
       std::vector<glm::vec3> vertices;
       std::vector<unsigned int> base_vertices;
       auto& meshes = model->getMeshes();
       for (auto& mesh : meshes) {
         base_vertices.push_back(vertices.size());
         vertices.insert(vertices.end(), mesh->getAABBVertices().begin(), mesh->getAABBVertices().end());
       }
       vertex_buffer->setData(&vertices[0], vertices.size() * sizeof(vertices[0]));
       GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, 0));
       for (unsigned int i = 0; i < meshes.size(); i++) {
         if (isFarAway(meshes[i], model_matrix)) {
           continue;
         }
         glm::vec3 color(1.f, 0.f, 0.f);
         if (meshes[i]->getSize() == Mesh::Size::MEDIUM) {
           color = glm::vec3(0.f, 1.f, 0.f);
         }
         else if (meshes[i]->getSize() == Mesh::Size::SMALL) {
           color = glm::vec3(0.f, 0.f, 1.f);
         }
         GL_CHECK(glUniform3f(shader->uniformLocation("col"), color.r, color.g, color.b));
         GL_CHECK(glDrawElementsBaseVertex(GL_LINES, indices.size(), GL_UNSIGNED_INT, nullptr, base_vertices[i]));
       }
     }
     GL_CHECK(glDisable(GL_DEPTH_TEST));
     _frontBuffer->setDepthTexture(nullptr);*/
  }

  void RenderingSystemOpenGL::renderParticles()
  {
    _lightingBuffer[0]->bind();
    _lightingBuffer[0]->setViewport();

    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    if (_billboards.size() || _terrainRenderables.size()) {
      GL_CHECK(glEnable(GL_DEPTH_TEST));
      GL_CHECK(glDepthMask(false));

      auto shader = _cloudBillboardShader;
      shader->bind();
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      bindTextureOrLoadAsync("assets/clouds.png");
      GL_CHECK(glUniform1i(shader->uniformLocation("cloudSampler"), 0));
      GL_CHECK(glActiveTexture(GL_TEXTURE1));
      _gBuffer->textures()[0]->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 1));
      GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
      for (auto& t : _terrainRenderables) {
        auto model_matrix = t.second->_transform->getModelMatrix();
        auto model_view = Mat4f(_viewMatrix) * model_matrix;
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
        for (auto& n : t.second->_visibleNodes) {
          if (n->_cloudBillboardPositionsAndScales.size()) {
            t.second->_cloudBillboardsVao[n]->bind();
            GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, n->_cloudBillboardPositionsAndScales.size()));
          }
        }
      }

      /* for (auto& b : _billboards) {
         auto billboard = b->getComponent<Billboard>();
         GL_CHECK(glActiveTexture(GL_TEXTURE0));
         bindTextureOrLoadAsync(billboard->getTexturePath());
         auto transform = b->getComponent<Transform>();
         auto model_view = _viewMatrix * transform->getModelMatrix();
         auto scale = transform->getScale();
         model_view[0][0] = scale.x;
         model_view[0][1] = 0.f;
         model_view[0][2] = 0.f;
         model_view[1][0] = 0.f;
         model_view[1][1] = scale.y;
         model_view[1][2] = 0.f;
         model_view[2][0] = 0.f;
         model_view[2][1] = 0.f;
         model_view[2][2] = scale.z;
         auto mvp = _projectionMatrix * model_view;
         GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
         GL_CHECK(glUniform4f(shader->uniformLocation("col"), 1.f, 1.f, 1.f, billboard->getOpacity()));
         GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
       }*/
      GL_CHECK(glDisable(GL_DEPTH_TEST));
      GL_CHECK(glDepthMask(true));
    }

    auto shader = _lensFlareShader;
    shader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    bindTextureOrLoadAsync("assets/lens_flare_ring.png");
    GL_CHECK(glUniform1i(shader->uniformLocation("lensFlareSampler"), 0));
    for (auto& light : _lights) {
      auto l = light->getComponent<Light>();
      float lens_flare_weight = l->_lensFlareWeight, ref_samples_passed = l->_lensFlareRefSamplesPassed;
      Vec3f l_pos_world = l->_pos;
     // auto l_pos_screen = _VP * light->getComponent<Transform>()->getModelMatrix() * glm::vec4(glm::vec3(0.f), 1.f);
      auto l_pos_screen = _VP * glm::vec4(l_pos_world[0], l_pos_world[1], l_pos_world[2], 1.f);
      l_pos_screen /= l_pos_screen.w;
      l_pos_screen = l_pos_screen * 0.5f + 0.5f;
      auto lens_end = 1.f - glm::vec3(l_pos_screen);
      lens_end = lens_end * 2.f - 1.f;
      l_pos_screen = l_pos_screen * 2.f - 1.f;
      glm::vec2 aspect_ratio(1.f, static_cast<float>(_viewportSize.x) / static_cast<float>(_viewportSize.y));
      const std::vector<float> offsets = { 0.2f, 0.5f, 0.8f, 0.33f, 0.56f, 0.66f };
      const std::vector<float> scales = { 1.f, 0.66f, 1.5f, 0.33f, 1.2f, 0.44f };
      std::vector<glm::vec3> colors = { glm::vec3(255.f, 196.f, 240.f) / 255.f, glm::vec3(209.f, 255.f, 202.f) / 255.f,
        glm::vec3(176.f, 193.f, 255.f) / 255.f,glm::vec3(255.f, 255.f, 193.f) / 255.f,glm::vec3(205.f, 255.f, 194.f) / 255.f,glm::vec3(255.f, 192.f, 193.f) / 255.f };
      glm::vec3 scale_factor = glm::vec3(aspect_ratio, 1.f) * glm::vec3(0.05f);
      float samples_passed = static_cast<float>(_lightQueries[light]->getResult());
      float weight = lens_flare_weight * glm::min(glm::smoothstep(0.f, ref_samples_passed, samples_passed), 0.065f);
      if (weight > 0.f) {
        GL_CHECK(glUniform1fv(shader->uniformLocation("scale"), scales.size(), &scales.front()));
        GL_CHECK(glUniform1fv(shader->uniformLocation("offset"), offsets.size(), &offsets.front()));
        GL_CHECK(glUniform3fv(shader->uniformLocation("uColor"), colors.size(), reinterpret_cast<float*>(&colors.front())));
        GL_CHECK(glUniform2f(shader->uniformLocation("scaleFactor"), scale_factor.x, scale_factor.y));
        GL_CHECK(glUniform2f(shader->uniformLocation("lensStart"), l_pos_screen.x, l_pos_screen.y));
        GL_CHECK(glUniform2f(shader->uniformLocation("lensEnd"), lens_end.x, lens_end.y));
        GL_CHECK(glUniform1f(shader->uniformLocation("weight"), weight));
        GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, scales.size()));
      }
    }
    GL_CHECK(glDisable(GL_BLEND));
  }

  void RenderingSystemOpenGL::renderWireFrame()
  {
    /* GL_CHECK(glClearColor(0.f, 0.f, 0.f, 0.f));
     GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
     _frontBuffer->bind();
     _frontBuffer->setViewport();

     GL_CHECK(glEnable(GL_DEPTH_TEST));
     _frontBuffer->setDepthTexture(_sceneDepthbuffer);

     GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
     glm::vec3 color(1.f, 1.f, 1.f);

     renderTerrain(TerrainRenderMode::WIREFRAME);

     for (auto& m : _models) {
       auto model = m->getComponent<Model>();
       auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
       auto mvp = _projectionMatrix * _viewMatrix * model_matrix;

       auto shader = _aabbShader;
       shader->bind();
       GL_CHECK(glUniform3f(shader->uniformLocation("col"), color.r, color.g, color.b));
       GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
       std::vector<std::shared_ptr<Mesh>> meshes;

       meshes = model->getMeshes();
       for (unsigned int i = 0; i < meshes.size(); i++) {
         //        if (isFarAway(meshes[i], model_matrix)) {
                  // continue;
               //  }
         setupMeshBindings(meshes[i]);
         GL_CHECK(glDrawElements(GL_TRIANGLES, meshes[i]->getIndices().size(), GL_UNSIGNED_INT, nullptr));
       }
     }

     GL_CHECK(glDisable(GL_DEPTH_TEST));
     _frontBuffer->setDepthTexture(nullptr);
     GL_CHECK(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
     GL_CHECK(glClearColor(_backGroundColor.r, _backGroundColor.g, _backGroundColor.b, _backGroundColor.a));*/
  }

  void RenderingSystemOpenGL::ssrPostProcess()
  {
    /* _gBuffer->textures()[0]->bind();
     GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D_ARRAY));
     _gBuffer->textures()[0]->setMagnificationFilter(GL_LINEAR);
     _gBuffer->textures()[0]->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);

     _backBuffer->bind();
     _backBuffer->setViewport();

     auto shader = _ssrShader;
     shader->bind();
     GL_CHECK(glActiveTexture(GL_TEXTURE0));
     _frontBuffer->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation("sceneSampler"), 0));
     GL_CHECK(glActiveTexture(GL_TEXTURE1));
     _gBuffer->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 1));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
     GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

     std::swap(_frontBuffer, _backBuffer);

     _gBuffer->textures()[0]->setMinificationFilter(GL_NEAREST);
     _gBuffer->textures()[0]->setMagnificationFilter(GL_NEAREST);*/
  }

  void RenderingSystemOpenGL::renderTrees(Entity* directional_light)
  {
    for (auto& t : _terrainRenderables) {
      const auto& tr = t.second;
      const auto& transform = tr->_transform;
      const auto& terrain = tr->_terrain;
      auto model_matrix = transform->getModelMatrix();
      auto tree_model_lod0 = terrain->getTreeModelLod0();
      auto tree_mesh_lod0 = tree_model_lod0->getMeshes()[0];
      auto tree_model_lod1 = terrain->getTreeModelLod1();
      auto tree_mesh_lod1 = tree_model_lod1->getMeshes()[0];
      auto leaf_model = terrain->getLeavesModel();
      auto leaf_mesh = leaf_model->getMeshes()[0];
      auto& wind_prm = terrain->getWindParams();
      if (directional_light) {
        std::vector<Mat4f> vp;
        auto dl = directional_light->getComponent<DirectionalLight>();
    //    auto transform = directional_light->getComponent<Transform>();
  //      auto pos_world_space = transform->getTranslation();
        dl->getViewProjectionMatrices(_aspectRatio, _zNear, _fov, inverse(glm::mat4(_viewMatrix)), dl->getViewMatrix(), _shadowMaps[directional_light]->width(), _settings._smFrustumSplits, vp);
        for (auto& n : t.second->_visibleNodesShadowMap) {
          if (n->_transforms.size() && n->_lod <= 1) {
            tr->_treeVao[n]->bind();
            auto shader = _treeShadowMapShader;
            shader->bind();
            GL_CHECK(glUniform1i(shader->uniformLocation("numLayers"), vp.size()));
            GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), vp.size(), false, vp[0].ptr()));
            GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("M"), 1, false, &model_matrix[0][0]));
            GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
            GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
            GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
            GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
            tr->_meshIbo->bind();
            GL_CHECK(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, tree_mesh_lod1->getIndices().size(), GL_UNSIGNED_INT, tr->_trunkLod1IdxOffset, n->_transforms.size(), tr->_trunkLod1BaseVertex));
#if PROFILE
            _treeTrianglesShadowMap += tree_mesh->getIndices().size() / 3 * n->_transforms.size() * vp.size();
#endif

            GL_CHECK(glDisable(GL_CULL_FACE));
            shader = _leavesShadowMapShader;
            shader->bind();
            GL_CHECK(glUniform1i(shader->uniformLocation("numLayers"), vp.size()));
            GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), vp.size(), false, &vp[0][0][0]));
            GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("M"), 1, false, &model_matrix[0][0]));
            GL_CHECK(glActiveTexture(GL_TEXTURE1));
            bindTextureOrLoadAsync(leaf_model->getMaterials()[leaf_mesh->getMaterialIndex()]->getDiffusePath());
            GL_CHECK(glUniform1i(shader->uniformLocation("samplerDiffuse"), 1));
            GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
            GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
            GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
            GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
            tr->_meshIbo->bind();
            GL_CHECK(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, leaf_mesh->getIndices().size(), GL_UNSIGNED_INT, tr->_leafsIdxOffs, n->_transforms.size(), tr->_leafsBaseVert));
            GL_CHECK(glEnable(GL_CULL_FACE));
#if PROFILE
            _treeTrianglesShadowMap += leaf_mesh->getIndices().size() / 3 * n->_transforms.size() * vp.size();
#endif
          }
        }
      }
      else {
        auto min = tree_mesh_lod0->getAABB()->getMin();
        auto max = tree_mesh_lod0->getAABB()->getMax();
        auto leaf_min = leaf_mesh->getAABB()->getMin();
        auto leaf_max = leaf_mesh->getAABB()->getMax();
        min = minimum(min, leaf_min);
        max = maximum(max, leaf_max);
        Vec3f scale = (max - min) * transform->getScale();
        auto& materials = tree_model_lod0->getMaterials();
        auto diffuse_color = tree_model_lod0->getMaterials()[tree_mesh_lod0->getMaterialIndex()]->getDiffuseColor();
        auto model_view = _viewMatrix * model_matrix;
        for (auto& n : t.second->_visibleNodes) {
          if (n->_transforms.size()) {
            tr->_treeVao[n]->bind();
            if (n->_lod <= 1) {
              // Draw trunk
              auto shader = _treeShader;
              shader->bind();
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVInverseTranspose"), 1, true, &inverse(glm::mat4(model_view))[0][0]));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
              GL_CHECK(glUniform1i(shader->uniformLocation("diffuseEnabled"), true));
              GL_CHECK(glUniform1i(shader->uniformLocation("normalEnabled"), true));
              GL_CHECK(glUniform1i(shader->uniformLocation("parallaxEnabled"), false));
              GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), false));
              GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), 0.f, 0.f, 0.f));
              GL_CHECK(glUniform3f(shader->uniformLocation("diffuseColor"), diffuse_color[0], diffuse_color[1], diffuse_color[2]));
              int tex = 0;
              GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
              bindTextureOrLoadAsync(materials[tree_mesh_lod0->getMaterialIndex()]->getDiffusePath());
              GL_CHECK(glUniform1i(shader->uniformLocation("samplerDiffuse"), tex));
              tex++;
              GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
              bindTextureOrLoadAsync(materials[tree_mesh_lod0->getMaterialIndex()]->getNormalPath());
              GL_CHECK(glUniform1i(shader->uniformLocation("samplerNormal"), tex));
              GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
              GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
              GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
              GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
              tr->_meshIbo->bind();
              if (n->_lod == 0) {
                GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, tree_mesh_lod0->getIndices().size(), GL_UNSIGNED_INT, 0, n->_transforms.size()));
              }
              else {
                GL_CHECK(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, tree_mesh_lod1->getIndices().size(), GL_UNSIGNED_INT, tr->_trunkLod1IdxOffset, n->_transforms.size(), tr->_trunkLod1BaseVertex));
              }
#if PROFILE
              _treeDrawCalls++;
              _treeTriangles += tree_mesh->getIndices().size() / 3 * n->_transforms.size();
#endif
              // Draw leaves
              GL_CHECK(glDisable(GL_CULL_FACE));
              shader = _leavesShader;
              shader->bind();
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVInverseTranspose"), 1, true, &inverse(glm::mat4(model_view))[0][0]));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
              GL_CHECK(glActiveTexture(GL_TEXTURE0));
              bindTextureOrLoadAsync(leaf_model->getMaterials()[leaf_mesh->getMaterialIndex()]->getDiffusePath());
              GL_CHECK(glUniform1i(shader->uniformLocation("samplerDiffuse"), 0));
              GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
              GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
              GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
              GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
              tr->_meshIbo->bind();
              GL_CHECK(glDrawElementsInstancedBaseVertex(GL_TRIANGLES, leaf_mesh->getIndices().size(), GL_UNSIGNED_INT, tr->_leafsIdxOffs, n->_transforms.size(), tr->_leafsBaseVert));
              GL_CHECK(glEnable(GL_CULL_FACE));
#if PROFILE
              _treeDrawCalls++;
              _treeTriangles += leaf_mesh->getIndices().size() / 3 * n->_transforms.size();
#endif
            }
            else {
              auto shader = _treeBillboardShader;
              shader->bind();
              GL_CHECK(glActiveTexture(GL_TEXTURE0));
              tr->_impostorFb[0]->textures()[0]->bind();
              GL_CHECK(glUniform1i(shader->uniformLocation("billboardSampler0"), 0));
              GL_CHECK(glActiveTexture(GL_TEXTURE1));
              tr->_impostorFb[1]->textures()[0]->bind();
              GL_CHECK(glUniform1i(shader->uniformLocation("billboardSampler1"), 1));
              GL_CHECK(glUniform1f(shader->uniformLocation("impostorAlpha"), _impostorAlpha));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
              GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
              GL_CHECK(glUniform3f(shader->uniformLocation("diffuseColor"), diffuse_color[0], diffuse_color[1], diffuse_color[2]));
              GL_CHECK(glUniform3f(shader->uniformLocation("modelScale"), scale[0], scale[1], scale[2]));
              GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
              GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
              GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
              GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
              GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, n->_transforms.size()));
#if PROFILE
              _treeDrawCalls++;
              _treeTriangles += 2 * n->_transforms.size();
#endif
            }
          }
        }
      }
    }
    // std::cout << "node size:" << n->_size << " transforms:" << n->_transforms.size() << " lod:" << n->_lod << std::endl;*/
  }

  void RenderingSystemOpenGL::renderImpostors()
  {
    _impostorAlpha = _frameCount % _impostorInterval / static_cast<float>(_impostorInterval);
    if (_frameCount == 1 || _frameCount % _impostorInterval == 0) {
      for (auto& t : _terrainRenderables) {
        const auto& terrain = t.second->_terrain;
        const auto& transform = t.second->_transform;
        for (auto& n : t.second->_visibleNodes) {
          if (n->_transforms.size()) {
            auto model_matrix = glm::mat4(transform->getModelMatrix()) * n->_transforms[0];
            t.second->renderImpostor(terrain->getTreeModelLod1(), terrain->getLeavesModel(), model_matrix, this);
            break;
          }
        }
      }
    }
  }

  void RenderingSystemOpenGL::createShader(std::shared_ptr<GLShaderProgram>& shader, const std::string & vs, const std::string & fs, const std::string & gs)
  {
    shader = std::make_shared<GLShaderProgram>();
    shader->create();
    shader->addShaderFromFile(vs, GLShaderProgram::ShaderType::VERTEX);
    if (gs != "") {
      shader->addShaderFromFile(gs, GLShaderProgram::ShaderType::GEOMETRY);
    }
    shader->addShaderFromFile(fs, GLShaderProgram::ShaderType::FRAGMENT);
    shader->link();
  }

  void RenderingSystemOpenGL::renderWater()
  {
    if (_terrainRenderables.size()) {
      auto& l0 = _lightingBuffer[0];
      auto& l1 = _lightingBuffer[1];

      GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, l0->id()));
      GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, l1->id()));
      GL_CHECK(glBlitFramebuffer(0, 0, l0->width(), l0->height(),
        0, 0, l1->width(), l1->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST));

      l0->bind();

      GL_CHECK(glEnable(GL_DEPTH_TEST));
      GL_CHECK(glDisable(GL_CULL_FACE));

      auto shader = _waterShader;
      shader->bind();

      auto light_entity = *_directionalLights.begin();
      auto dl = light_entity->getComponent<DirectionalLight>();
    //  auto light_transform = light_entity->getComponent<Transform>();
      auto l_pos_world = dl->_pos;
      auto light_pos_view_space = _viewMatrix * glm::vec4(l_pos_world[0], l_pos_world[1], l_pos_world[2], 1.f);
      GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), dl->_color[0], dl->_color[1], dl->_color[2]));
      GL_CHECK(glUniform3f(shader->uniformLocation("lightPosViewSpace"), light_pos_view_space[0], light_pos_view_space[1], light_pos_view_space[2]));
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      bindTextureOrLoadAsync("assets/water_normals.png");
      GL_CHECK(glUniform1i(shader->uniformLocation("normalSampler"), 0));
      GL_CHECK(glActiveTexture(GL_TEXTURE1));
      _gBuffer->textures()[0]->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 1));
      GL_CHECK(glActiveTexture(GL_TEXTURE2));
      l1->textures()[0]->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("lightingSampler"), 2));
      GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
      for (auto& t : _terrainRenderables) {
        Mat4f model_matrix = t.second->getWaterModelMatrix();
        glm::mat4 model_view = _viewMatrix * model_matrix;
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
        glm::vec3 normal_ms(0.f, 1.f, 0.f);
        glm::vec3 normal_cs = normalize(glm::vec3(transpose(inverse(glm::mat4(model_view))) * glm::vec4(normal_ms, 0.f)));
        GL_CHECK(glUniform3f(shader->uniformLocation("normalViewSpace"), normal_cs.x, normal_cs.y, normal_cs.z));
        glm::vec3 tangent_ms(1.f, 0.f, 0.f);
        glm::vec3 bitangent_ms(0.f, 0.f, 1.f);
        glm::mat3 TBN_view(glm::normalize(glm::vec3(model_view * glm::vec4(tangent_ms, 0.f))), glm::normalize(glm::vec3(model_view * glm::vec4(bitangent_ms, 0.f))), glm::normalize(glm::vec3(model_view * glm::vec4(normal_ms, 0.f))));
        GL_CHECK(glUniformMatrix3fv(shader->uniformLocation("TBN_view"), 1, false, &TBN_view[0][0]));
        GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
      }

      GL_CHECK(glEnable(GL_CULL_FACE));
      GL_CHECK(glDisable(GL_DEPTH_TEST));
    }
  }

  void RenderingSystemOpenGL::renderDirectionalLights()
  {
    auto shader = _directionalLightShader;
    shader->bind();

    float aspect = static_cast<float>(_viewportSize.x) / static_cast<float>(_viewportSize.y);
   // auto mat = glm::scale(glm::vec3(0.03f)) * glm::scale(glm::vec3(1.f, aspect, 1.f));
    auto mat = scale<4, float>(Vec3f(0.03f)) * scale<4, float>(Vec3f({ 1.f, aspect, 1.f }));
    for (auto& l : _directionalLights) {
      auto dl = l->getComponent<DirectionalLight>();
      auto light_pos_cs = _viewMatrix * glm::vec4(dl->_pos[0], dl->_pos[1], dl->_pos[2], 1.f);
      if (light_pos_cs[2] <= -_zNear) {
        auto light_pos_screen = _projectionMatrix * light_pos_cs;
        light_pos_screen /= light_pos_screen[3];
       // auto mvp = glm::translate(glm::vec3(light_pos_screen.x, light_pos_screen.y, 1.f)) * mat;
        auto mvp = translate<4, float>(Vec3f({ light_pos_screen[0], light_pos_screen[1], 1.f })) * mat;
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
        auto col = dl->_color;
        GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), col[0], col[1], col[2]));
        if (_lensFlaresEnabled) {
          _lightQueries[l]->begin();
        }
        GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        if (_lensFlaresEnabled) {
          _lightQueries[l]->end();
        }
      }
    }
  }

  /* void RenderingSystemOpenGL::renderWaterForward(WaterRenderable * water_renderable)
   {
     GL_CHECK(glDisable(GL_CULL_FACE));

     auto shader = _shaderProgramWaterForward;
     shader->bind();

     auto model_matrix = water_renderable->getModelMatrix();
     auto mv = _viewMatrix * model_matrix;
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation( "MV"), 1, false, &mv[0][0]));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation( "V_inverse"), 1, false, &inverse(_viewMatrix)[0][0]));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation( "P"), 1, false, &_projectionMatrix[0][0]));
     auto mv_inverse_transpose = glm::transpose(glm::inverse(mv));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation( "MVInverseTranspose"), 1, false, &mv_inverse_transpose[0][0]));

     auto light_pos_world = _lights.front()->getWorldSpacePosition();
     auto light_pos_view_space = _viewMatrix * glm::vec4(light_pos_world, 1.f);
     GL_CHECK(glUniform3f(shader->uniformLocation( "lightPosViewSpace"), light_pos_view_space.x, light_pos_view_space.y, light_pos_view_space.z));
     auto light_color = _lights.front()->_color;
     if (_gammaCorrectionEnabled) {
       light_color = glm::pow(light_color, glm::vec3(2.2f));
     }
     GL_CHECK(glUniform3f(shader->uniformLocation( "lightColor"), light_color.r, light_color.g, light_color.b));
     GL_CHECK(glUniform1i(shader->uniformLocation( "doNormalMapping"), _normalMappingEnabled));
     GL_CHECK(glUniform1i(shader->uniformLocation( "doGammaCorrection"), _gammaCorrectionEnabled));
     GL_CHECK(glUniform1f(shader->uniformLocation( "uvFrequency"), water_renderable->_uvFrequency));
     GL_CHECK(glUniform1f(shader->uniformLocation( "waveStrength"), water_renderable->_waveStrength));
     GL_CHECK(glUniform1f(shader->uniformLocation( "waveFrequency"), water_renderable->_waveFrequency));
     GL_CHECK(glUniform1f(shader->uniformLocation( "fadeDist"), water_renderable->_fadeDist));

     glActiveTexture(GL_TEXTURE0);
     _readBuffer->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerShaded"), 0));

     glActiveTexture(GL_TEXTURE1);
     if (_textures[water_renderable->_normalMap] == nullptr) {
       addTexture(water_renderable->_normalMap);
     }
     _textures[water_renderable->_normalMap]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerNormal"), 1));

     glActiveTexture(GL_TEXTURE2);
     if (_textures[water_renderable->_duDvMap] == nullptr) {
       addTexture(water_renderable->_duDvMap);
     }
     _textures[water_renderable->_duDvMap]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerDuDv"), 2));

     glActiveTexture(GL_TEXTURE3);
     _skyboxTexture->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerSkybox"), 3));

     glActiveTexture(GL_TEXTURE4);
     _gBuffer->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerPosCs"), 4));

     glActiveTexture(GL_TEXTURE5);
     _gBufferBackface->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerPosCsBack"), 5));

     glActiveTexture(GL_TEXTURE6);
     _gBuffer->textures()[3]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerOcclusion"), 6));

     glActiveTexture(GL_TEXTURE7);
     if (_textures[water_renderable->_splashTexture] == nullptr) {
       _textures[water_renderable->_splashTexture] = std::make_shared<GLTexture>(GL_TEXTURE_2D);
       _textures[water_renderable->_splashTexture]->create();
       _textures[water_renderable->_splashTexture]->bind();
       _textures[water_renderable->_splashTexture]->setMagnificationFilter(GL_LINEAR);
       _textures[water_renderable->_splashTexture]->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
       GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
       GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
       float color[] = { 128.f / 255.f, 128.f / 255.f, 255.f, 255.f };
       GL_CHECK(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color));
       _textures[water_renderable->_splashTexture]->setData(water_renderable->_splashTexture->cols, water_renderable->_splashTexture->rows, GL_RGB32F, GL_BGR, GL_UNSIGNED_BYTE, water_renderable->_splashTexture->data);
       GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
     }
     _textures[water_renderable->_splashTexture]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation( "samplerWaterSplash"), 7));

     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation( "PPixelSpace"), 1, false, &_projectionPixelSpace[0][0]));
     GL_CHECK(glUniform1f(shader->uniformLocation( "nearPlaneZ"), _zNear));
     GL_CHECK(glUniform1f(shader->uniformLocation( "maxSteps"), _raytracingStepsReflections));
     GL_CHECK(glUniform1i(shader->uniformLocation( "doSSR"), _ssrEnabled));
     GL_CHECK(glUniform1f(shader->uniformLocation( "time"), _window->getTime()));

     if (water_renderable->_splashes.size() > 0) {
       std::vector<glm::vec2> splash_pos;
       std::vector<float> splash_scale;
       std::vector<float> splash_weight;
       for (auto& s : water_renderable->_splashes) {
         splash_pos.push_back(s._pos);
         splash_scale.push_back(s._scale * s._age);
         splash_weight.push_back(1.f - s._age);
       }
       GL_CHECK(glUniform2fv(shader->uniformLocation( "splashPos"), splash_pos.size(), reinterpret_cast<float*>(&splash_pos[0])));
       GL_CHECK(glUniform1fv(shader->uniformLocation( "splashScale"), splash_scale.size(), &splash_scale[0]));
       GL_CHECK(glUniform1fv(shader->uniformLocation( "splashWeight"), splash_weight.size(), &splash_weight[0]));
     }

     GL_CHECK(glUniform1i(shader->uniformLocation( "numSplashes"), water_renderable->_splashes.size()));

     setupVertices(water_renderable->_mesh);
     setupIndices(water_renderable->_mesh);

     GL_CHECK(glDrawElements(GL_TRIANGLES, water_renderable->_mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));

     GL_CHECK(glEnable(GL_CULL_FACE));
   }*/

#if PROFILE
  unsigned int RenderingSystemOpenGL::getTreeDrawCalls()
  {
    return _treeDrawCalls;
  }
  unsigned int RenderingSystemOpenGL::getTreeTriangles()
  {
    return _treeTriangles;
  }
  unsigned int RenderingSystemOpenGL::getTreeTrianglesShadowMap()
  {
    return _treeTrianglesShadowMap;
  }
  unsigned int RenderingSystemOpenGL::getGrassDrawCalls()
  {
    return _grassDrawCalls;
  }
  unsigned int RenderingSystemOpenGL::getGrassBlades()
  {
    return _grassBlades;
  }
  unsigned int RenderingSystemOpenGL::getGrassTriangles()
  {
    return _grassBlades * 5;
  }
  RenderingSystemOpenGL::Timing RenderingSystemOpenGL::getTiming(RenderStage stage)
  {
    return _timings[stage];
  }
#endif

  void RenderingSystemOpenGL::prepareRender(float delta_time, float time)
  {
    _frameCount++;
    _time = time;
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthFunc(GL_LEQUAL));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glClearColor(_backGroundColor.r, _backGroundColor.g, _backGroundColor.b, _backGroundColor.a));
    if (_cameras.size() == 0) {
      std::cout << "No camera found, please add a camera component to some entity" << std::endl;
      return;
    }
    auto active_camera = (*_cameras.begin())->getComponent<Camera>();
    for (auto& c : _cameras) {
      if (c->getComponent<Camera>()->_isActive) {
        active_camera = c->getComponent<Camera>();
        break;
      }
    }
    const float target_fps = 60.f;
    _fpsFactor = delta_time > 0.f ? glm::mix(1.f / delta_time / target_fps, _fpsFactor, _lerpAlpha) : 1.f;
    glm::vec3 euler_angles;
    if (_smoothCamera) {
      _camPos = glm::mix(active_camera->_pos, _camPosBefore, _lerpAlpha);
      glm::quat q_before(_eulerAnglesBefore);
      glm::quat q_now(active_camera->_eulerAngles);
      auto q_new = glm::slerp(q_before, q_now, 1.f - _lerpAlpha);
      euler_angles = eulerAngles(q_new);
    }
    else {
      _camPos = active_camera->_pos;
      euler_angles = active_camera->_eulerAngles;
    }

    _viewMatrix = active_camera->getViewMatrix(_camPos, euler_angles);
    _eulerAnglesBefore = euler_angles;

    _projectionMatrix = getProjectionMatrix(_viewportSize.x, _viewportSize.y);
    _VP = _projectionMatrix * _viewMatrix;
    _activeCamera = active_camera;

    auto dl = (*_directionalLights.begin())->getComponent<DirectionalLight>();
    auto shadow_map = _shadowMaps[*_directionalLights.begin()];
    std::vector<Mat4f> vp;
    dl->getViewProjectionMatrices(_aspectRatio, _zNear, _fov, inverse(_viewMatrix), dl->getViewMatrix(), shadow_map->width(), _settings._smFrustumSplits, vp);
    for (auto& t : _terrainRenderables) {
      t.second->_visibleNodes.clear();
      t.second->_visibleNodesShadowMap.clear();
      auto model_matrix = t.second->_transform->getModelMatrix();
      t.second->_terrain->getTreeNodesForRendering(glm::inverse(glm::mat4(model_matrix)) * glm::vec4(_camPos, 1.f), t.second->_visibleNodes, _VP * glm::mat4(model_matrix), nullptr);
      std::vector<Mat4f> light_mvps;
      for (unsigned i = 0; i < _settings._smFrustumSplits.size(); ++i) {
        light_mvps.push_back(vp[i] * model_matrix);
      }
      t.second->_terrain->getTreeNodesForRendering(glm::inverse(glm::mat4(model_matrix)) * glm::vec4(_camPos, 1.f), t.second->_visibleNodesShadowMap, _VP * glm::mat4(model_matrix), (*_directionalLights.begin())->getComponent<DirectionalLight>().get(), light_mvps);
    }

#if PROFILE
    _treeDrawCalls = 0;
    _treeTriangles = 0;
    _treeTrianglesShadowMap = 0;
    _grassDrawCalls = 0;
    _grassBlades = 0;
#endif
  }

  void RenderingSystemOpenGL::renderShadowMaps()
  {
#if PROFILE
    _timings[RenderStage::SHADOWMAP] = Timing();
#endif
    if (_shadowsEnabled || _lightVolumesEnabled) {
      GL_CHECK(glEnable(GL_DEPTH_CLAMP));
      for (auto& dl : _directionalLights) {
        renderShadowMapsDirectional(dl);
      }
      GL_CHECK(glDisable(GL_DEPTH_CLAMP));
      for (auto& sl : _spotLights) {
        renderShadowMapsSpot(sl);
      }
      for (auto& pl : _pointLights) {
        renderShadowMapsPoint(pl);
      }
    }
#if PROFILE
    _timings[RenderStage::SHADOWMAP].stop();
#endif
  }

  void RenderingSystemOpenGL::postProcessing(float delta_time)
  {
    /* if (_renderAABBs) {
       renderAABBs();
     }

     if (_ssrEnabled) {
       ssrPostProcess();
     }*/

    if (_lensFlaresEnabled) {
      renderParticles();
    }

    if (_bloomEnabled) {
      bloomPostProcess();
    }

    if (_godRaysEnabled) {
      godRayPostProcess();
    }



    if (_depthOfFieldEnabled) {
      depthOfFieldPostProcess();
    }

    /* if (_lightVolumesEnabled) {
       if (_lightVolumeBilateralBlur) {
         computeGradient();
       }
       _sceneDepthbuffer->bind();
       GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
       volumeLightPostProcess();
     }*/

    if (_eyeAdaptionEnabled) {
      eyeAdaptionPostProcess();
    }
  }

  void RenderingSystemOpenGL::swapParams()
  {
    _VPBefore = _VP;
    _camPosBefore = _camPos;
    std::swap(_exposureBuffer[0], _exposureBuffer[1]);
  }

  void RenderingSystemOpenGL::renderToScreen()
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebufferId));
    GL_CHECK(glViewport(0, 0, _viewportSize.x, _viewportSize.y));
    auto shader = _shaderProgramScreen;
    shader->bind();
    int tex = 0;
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
    _lightingBuffer[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("lightingSampler"), tex));
    tex++;
    if (_bloomEnabled) {
      for (unsigned int i = 0; i < _bloomStages.size(); i++) {
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        _bloomStages[i]._result->bind();
        GL_CHECK(glUniform1i(shader->uniformLocation("bloomSampler" + std::to_string(i)), tex));
        tex++;
      }
      GL_CHECK(glUniform1fv(shader->uniformLocation("bloomWeights"), _bloomWeights.size(), &_bloomWeights.front()));
    }
    GL_CHECK(glUniform1i(shader->uniformLocation("bloomEnabled"), _bloomEnabled));
    if (_renderGodRays) {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      _godRayEffect->_resultFb->textures()[0]->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("godRaySampler"), tex));
      GL_CHECK(glUniform1f(shader->uniformLocation("godRayWeight"), _godRayWeight));
      tex++;
    }
    GL_CHECK(glUniform1i(shader->uniformLocation("renderGodRays"), _renderGodRays && _godRaysEnabled));
    if (_depthOfFieldEnabled) {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      _DOFBlurBuffer[0]->textures()[0]->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("dofSampler"), tex));
      GL_CHECK(glUniform3f(shader->uniformLocation("dofParam"), _dofParam.x, _dofParam.y, _dofParam.z));
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("PInverse"), 1, false, &inverse(_projectionMatrix)[0][0]));
      tex++;
    }
    GL_CHECK(glUniform1i(shader->uniformLocation("dofEnabled"), _depthOfFieldEnabled));
    if (_depthOfFieldEnabled || _motionBlurEnabled) {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      _sceneDepthbuffer->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("depthSampler"), tex));
      tex++;
    }
    if (_motionBlurEnabled) {
      GL_CHECK(glUniform1f(shader->uniformLocation("motionBlurStrength"), _motionBlurStrength * _fpsFactor));
      auto d_mat = glm::dmat4(_VPBefore) * glm::inverse(glm::dmat4(glm::mat4(_viewMatrix))) * glm::inverse(glm::dmat4(glm::mat4(_projectionMatrix)));
      auto mat = glm::mat4(d_mat);
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VPBeforeTimesVPInverse"), 1, false, &mat[0][0]));
    }
    GL_CHECK(glUniform1i(shader->uniformLocation("motionBlurEnabled"), _motionBlurEnabled));
    for (auto& t : _terrainRenderables) {
      auto model_matrix = t.second->getWaterModelMatrix();
      auto world_height = model_matrix[3][1];
      auto vp_inverse = inverse(_viewMatrix) * inverse(Mat4f(_projectionMatrix));
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP_inverse"), 1, false, &vp_inverse[0][0]));
      GL_CHECK(glUniform1f(shader->uniformLocation("waterHeight"), world_height));
      if (!_popupAnimate && _camPosBefore.y < world_height && _camPos.y >= world_height) {
        _popupStart = _time;
        _popupEnd = _time + 2.5f;
        _popupAnimate = true;
      }
      if (_popupAnimate) {
        if (_time <= _popupEnd) {
          float alpha = glm::smoothstep(_popupStart, _popupEnd, _time);
          GL_CHECK(glUniform1f(shader->uniformLocation("waterDistortionAlpha"), alpha));
          GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
        }
        else {
          _popupAnimate = false;
        }
      }
      GL_CHECK(glUniform1i(shader->uniformLocation("waterDistortion"), _popupAnimate));
      break;
    }
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
    _exposureBuffer[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerExposure"), tex));
    tex++;
    GL_CHECK(glUniform1i(shader->uniformLocation("gammaCorrectionEnabled"), _gammaCorrectionEnabled));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }

  void RenderingSystemOpenGL::geometryPass()
  {
    _gBuffer->bind();
    _gBuffer->setViewport();
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
    std::vector<float> pos_clear_color(4, std::numeric_limits<float>::lowest());
    GL_CHECK(glClearBufferfv(GL_COLOR, 0, &pos_clear_color[0])); // position
    std::vector<float> zero_clear_color(4, 0.f);
    GL_CHECK(glClearBufferfv(GL_COLOR, 1, &zero_clear_color[0])); // normals
    GL_CHECK(glClearBufferfv(GL_COLOR, 2, &_backGroundColor[0])); //diffuse color
    GL_CHECK(glClearBufferfv(GL_COLOR, 3, &_backGroundColor[0])); // occlusion
    std::vector<float> spec_color(4, 1.f);
    GL_CHECK(glClearBufferfv(GL_COLOR, 4, &spec_color[0])); // specular exponent
    GL_CHECK(glClearBufferfv(GL_COLOR, 5, &zero_clear_color[0])); // light source
#if PROFILE
    _timings[RenderStage::MODELS] = Timing();
#endif
    renderModels();
#if PROFILE
    _timings[RenderStage::MODELS].stop();
    _timings[RenderStage::TERRAIN] = Timing();
#endif
    renderTerrain(TerrainRenderMode::NORMAL);

#if PROFILE
    _timings[RenderStage::TREES] = Timing();
#endif
    renderTrees(nullptr);
#if PROFILE
    _timings[RenderStage::TREES].stop();
#endif
#if PROFILE
    _timings[RenderStage::TERRAIN].stop();
    _timings[RenderStage::GRASS] = Timing();
#endif
    renderTerrain(TerrainRenderMode::GRASS);
#if PROFILE
    _timings[RenderStage::GRASS].stop();
#endif
    //  renderLightSources();
    renderSkybox();
    renderSkydome();
    renderDirectionalLights();
    GL_CHECK(glDisable(GL_DEPTH_TEST));
  }

  void RenderingSystemOpenGL::renderModels()
  {
    auto shader = _shaderProgramDeferredGeometryTextured;
    shader->bind();
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
    std::vector<Entity*> models(_models.begin(), _models.end());
    models.erase(std::remove_if(models.begin(), models.end(), [this](Entity* entity) {
      return _lights.count(entity);
    }), models.end());
    for (auto& m : models) {
      auto transform = m->getComponent<Transform>();
      auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
      auto model_view = Mat4f(_viewMatrix) * model_matrix;
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVInverseTranspose"), 1, true, &inverse(glm::mat4(model_view))[0][0]));
      auto cam_pos_model_space = inverse(model_matrix) * glm::vec4(_camPos, 1.f);
      GL_CHECK(glUniform3f(shader->uniformLocation("camPosModelSpace"), cam_pos_model_space[0], cam_pos_model_space[1], cam_pos_model_space[2]));
      auto model = m->getComponent<Model>();
      auto meshes = model->getMeshes();
      meshes.erase(std::remove_if(meshes.begin(), meshes.end(), [this, &model_matrix](const std::shared_ptr<Mesh>& mesh) {
        return isCulled(mesh, model_matrix);
      }), meshes.end());
      auto& materials = model->getMaterials();
      unsigned int material_index = materials.size();
      for (auto& mesh : meshes) {
        auto material_index_new = mesh->getMaterialIndex();
        if (material_index != material_index_new) {
          int tex = 0;
          auto material = materials[material_index_new];
          auto& diffuse_path = material->getDiffusePath();
          if (diffuse_path != "") {
            GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
            bindTextureOrLoadAsync(diffuse_path);
            GL_CHECK(glUniform1i(shader->uniformLocation("samplerDiffuse"), tex));
            tex++;
          }
          auto& opacity_path = material->getOpacityPath();
          if (opacity_path != "") {
            GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
            bindTextureOrLoadAsync(opacity_path);
            GL_CHECK(glUniform1i(shader->uniformLocation("samplerOpacity"), tex));
            tex++;
          }
          auto& normal_path = material->getNormalPath();
          if (normal_path != "") {
            GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
            bindTextureOrLoadAsync(normal_path);
            GL_CHECK(glUniform1i(shader->uniformLocation("samplerNormal"), tex));
            tex++;
          }
          GL_CHECK(glUniform1f(shader->uniformLocation("specularExponent"), material->getSpecularExponent()));
          GL_CHECK(glUniform1i(shader->uniformLocation("diffuseEnabled"), diffuse_path != "" && _textures[diffuse_path] != nullptr));
          GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), opacity_path != "" && _textures[opacity_path] != nullptr));
          GL_CHECK(glUniform1i(shader->uniformLocation("normalEnabled"), _normalMappingEnabled && normal_path != "" && _textures[normal_path] != nullptr));
          GL_CHECK(glUniform1i(shader->uniformLocation("parallaxEnabled"), false));

          auto& diffuse_color = material->getDiffuseColor();
          glm::vec3 light_color(0.f);
          GL_CHECK(glUniform3f(shader->uniformLocation("diffuseColor"), diffuse_color[0], diffuse_color[1], diffuse_color[2]));
          GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), light_color.x, light_color.y, light_color.z));
        }
        setupMeshBindings(mesh);
        //std::cout << "Draw mesh" << std::endl;
        GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
        material_index = material_index_new;
      }
    }
  }

  void RenderingSystemOpenGL::renderTerrain(RenderingSystemOpenGL::TerrainRenderMode mode)
  {
    std::shared_ptr<GLShaderProgram> shader;
    if (mode == TerrainRenderMode::NORMAL) {
      shader = _terrainShader;
    }
    else if (mode == TerrainRenderMode::WIREFRAME) {
      shader = _terrainWireframeShader;
    }
    else {
      GL_CHECK(glDisable(GL_CULL_FACE));
      shader = _grassShader;
    }
    shader->bind();
    if (mode != TerrainRenderMode::WIREFRAME) {
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
    }
    else {
      GL_CHECK(glUniform3f(shader->uniformLocation("col"), 1.f, 1.f, 1.f));
    }
    for (auto& t : _terrainRenderables) {
      auto terrain_renderable = t.second;
      auto model_matrix = terrain_renderable->_transform->getModelMatrix();
      auto model_view = _viewMatrix * model_matrix;
      if (mode != TerrainRenderMode::WIREFRAME) {
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
      }
      else {
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &(Mat4f(_projectionMatrix) * model_view)[0][0]));
      }
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVInverseTranspose"), 1, true, &inverse(model_view)[0][0]));
      if (mode == TerrainRenderMode::GRASS) {
        auto& wind_prm = terrain_renderable->_terrain->getWindParams();
        GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
        GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_prm._dir.x, wind_prm._dir.y));
        GL_CHECK(glUniform1f(shader->uniformLocation("windStrength"), wind_prm._strength));
        GL_CHECK(glUniform1f(shader->uniformLocation("windFrequency"), wind_prm._frequency));
      }
      int tex = 0;
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      terrain_renderable->_heightMap->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("heightMapSampler"), tex));
      tex++;
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      terrain_renderable->_normalMap->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("normalSampler"), tex));
      tex++;
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
      terrain_renderable->_splatMap->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("grassDistSampler"), tex));
      tex++;

      const auto& terrain = terrain_renderable->_terrain;
      auto grass_color_high = terrain->getGrassColor();
      GL_CHECK(glUniform3f(shader->uniformLocation("grassColorHigh"), grass_color_high.r, grass_color_high.g, grass_color_high.b));
      auto terrain_color = terrain->getTerrainColor();
      GL_CHECK(glUniform3f(shader->uniformLocation("terrainColor"), terrain_color.r, terrain_color.g, terrain_color.b));
      float grass_height = terrain->getGrassHeight();
      GL_CHECK(glUniform1f(shader->uniformLocation("grassHeight"), grass_height));
      int blades_per_unit = 32;
      GL_CHECK(glUniform1f(shader->uniformLocation("bladesPerUnit"), blades_per_unit));
      if (mode == TerrainRenderMode::GRASS) {
        auto cam_pos_model_space = glm::round(glm::mat4(inverse(model_matrix)) * glm::vec4(_camPos, 1.f));
        float grass_distance = _grassQuadTree->getSize() * 2.f;
        GL_CHECK(glUniform1f(shader->uniformLocation("grassDistance"), grass_distance));
        auto mvp = Mat4f(_projectionMatrix) * model_view;
        auto quadtree_offset = -_grassQuadTree->getSize() / 2 + glm::ivec2(cam_pos_model_space.x, cam_pos_model_space.z);
        for (auto& n : _grassQuadTree->getNodes()) {
          auto node_pos_model_space = n->_pos + quadtree_offset;
          int patch_size = n->_size;
          auto node_center_model_space = node_pos_model_space + patch_size / 2;
          auto node_center_view_space = model_view * glm::vec4(node_center_model_space.x,
            terrain->getHeight(node_center_model_space.x, node_center_model_space.y), node_center_model_space.y, 1.f);
          if (node_center_view_space.length() > grass_distance * 1.5f) {
            continue;
          }
          /* This is an approximation
          ----------------------------------------------------------------
          */
          float height_south_west = terrain->getHeight(node_pos_model_space.x, node_pos_model_space.y);
          float height_south_east = terrain->getHeight(node_pos_model_space.x + patch_size, node_pos_model_space.y);
          float height_north_west = terrain->getHeight(node_pos_model_space.x, node_pos_model_space.y + patch_size);
          float height_north_east = terrain->getHeight(node_pos_model_space.x + patch_size, node_pos_model_space.y + patch_size);
          float min_height = std::min(height_south_west, std::min(height_south_east, std::min(height_north_west, height_north_east)));
          float max_height = std::max(height_south_west + grass_height, std::max(height_south_east + grass_height, std::max(height_north_west + grass_height, height_north_east + grass_height)));
          /*
          ----------------------------------------------------------------
          */
          Vec3f bb_min({ static_cast<float>(node_pos_model_space.x),static_cast<float>(min_height), static_cast<float>(node_pos_model_space.y) });
          Vec3f bb_max({ static_cast<float>(node_pos_model_space.x + patch_size), max_height, static_cast<float>(node_pos_model_space.y + patch_size) });

          if (!AABB(bb_min, bb_max).isVisible<false, false>(mvp)) {
            continue;
          }
          GL_CHECK(glUniform2f(shader->uniformLocation("grassStart"), node_pos_model_space.x, node_pos_model_space.y));
          int blades_per_dir = blades_per_unit * patch_size;
          GL_CHECK(glUniform1i(shader->uniformLocation("patchSize"), patch_size));
          GL_CHECK(glUniform1i(shader->uniformLocation("bladesPerDir"), blades_per_dir));

          int num_blades = blades_per_dir * blades_per_dir;;
          GL_CHECK(glDrawArrays(GL_POINTS, 0, num_blades));

#if PROFILE
          _grassBlades += num_blades;
          _grassDrawCalls++;
#endif
        }
      }
      else {
        terrain_renderable->_terrainVao->bind();
        std::map<int, std::map<int, int>> lods;
        auto cam_pos_model_space = inverse(glm::mat4(model_matrix)) * glm::vec4(_camPos, 1.f);
        terrain->getTilesForRendering(lods, cam_pos_model_space, glm::mat4(_projectionMatrix * model_view));
        GL_CHECK(glUniform1f(shader->uniformLocation("scale"), 1.f));
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/mountainslab01.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("rocksSampler"), tex));
        tex++;
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/mountainslab01_n.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("rocksNormalSampler"), tex));
        tex++;
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/pineforest03.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("grassSampler"), tex));
        tex++;
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/pineforest03_n.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("grassNormalSampler"), tex));
        tex++;
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/road01.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("roadSampler"), tex));
        tex++;
        GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex));
        bindTextureOrLoadAsync("assets/road01_n.dds");
        GL_CHECK(glUniform1i(shader->uniformLocation("roadNormalSampler"), tex));
        tex++;
        for (auto& e1 : lods) {
          for (auto& e2 : e1.second) {
            GL_CHECK(glUniform2f(shader->uniformLocation("pos"), e1.first, e2.first));
            int lod = e2.second;
            int dir = NONE;
            {
              int north_z = e2.first + terrain->getTileSize();
              if (e1.second.count(north_z) && lod < e1.second[north_z]) {
                dir |= NORTH;
              }
            }
            {
              int south_z = e2.first - terrain->getTileSize();
              if (e1.second.count(south_z) && lod < e1.second[south_z]) {
                dir |= SOUTH;
              }
            }
            {
              int west_x = e1.first - terrain->getTileSize();
              if (lods.count(west_x) && lod < lods[west_x][e2.first]) {
                dir |= WEST;
              }
            }
            {
              int east_x = e1.first + terrain->getTileSize();
              if (lods.count(east_x) && lod < lods[east_x][e2.first]) {
                dir |= EAST;
              }
            }
            terrain_renderable->_terrainIbo[lod][dir]->bind();
            GL_CHECK(glDrawElements(GL_TRIANGLES, terrain_renderable->_terrainNumIndices[lod][dir], GL_UNSIGNED_INT, nullptr));
          }
        }
      }
    }
    if (mode == TerrainRenderMode::GRASS) {
      GL_CHECK(glEnable(GL_CULL_FACE));
    }
  }

  void RenderingSystemOpenGL::renderLightSources()
  {
    GL_CHECK(glEnable(GL_DEPTH_CLAMP));
    auto shader = _shaderProgramDeferredGeometryTextured;
    shader->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("diffuseEnabled"), false));
    GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), false));
    GL_CHECK(glUniform1i(shader->uniformLocation("normalEnabled"), false));
    GL_CHECK(glUniform1i(shader->uniformLocation("parallaxEnabled"), false));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P"), 1, false, &_projectionMatrix[0][0]));
    for (auto& l : _lights) {
      if (_lensFlaresEnabled) {
        _lightQueries[l]->begin();
      }
      auto light = l->getComponent<Light>();
      glm::vec3 color = light->_color;
      Vec3f pos = light->_pos;
      GL_CHECK(glUniform3f(shader->uniformLocation("diffuseColor"), color.r, color.g, color.b));
      GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), color.r, color.g, color.b));
      auto model = l->getComponent<Model>();
      auto model_view = _viewMatrix * Transform(pos).getModelMatrix();
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MV"), 1, false, &model_view[0][0]));
      for (auto& mesh : model->getMeshes()) {
        setupMeshBindings(mesh);
        GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
      }
      if (_lensFlaresEnabled) {
        _lightQueries[l]->end();
      }
    }
    GL_CHECK(glDisable(GL_DEPTH_CLAMP));
  }

  void RenderingSystemOpenGL::lightingDirectional(Entity* directional_light)
  {
    auto shader = _shaderProgramDeferredLightingDirectional;
    shader->bind();

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _gBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 0));

    auto light = directional_light->getComponent<DirectionalLight>();
    auto light_pos_world = light->_pos;
    auto csm_distances = _settings._smFrustumSplits;
    auto shadow_map = _shadowMaps[directional_light];
    std::vector<Mat4f> vp;
    std::vector<Mat4f> v_inverse_vp_light;
    light->getViewProjectionMatrices(_aspectRatio, _zNear, _fov, inverse(_viewMatrix), light->getViewMatrix(), shadow_map->width(), _settings._smFrustumSplits, vp);
    for (unsigned int i = 0; i < vp.size(); i++) {
      v_inverse_vp_light.push_back(vp[i] * Mat4f(inverse(_viewMatrix)));
    }

    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    shadow_map->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowMapSampler"), 1));

    GL_CHECK(glUniform1i(shader->uniformLocation("numSamples"), _shadowPCFSamples));

    glm::vec2 sm_texel_size = 1.f / glm::vec2(shadow_map->width());
    GL_CHECK(glUniform2f(shader->uniformLocation("smTexelSize"), sm_texel_size.x, sm_texel_size.y));

    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse_VPLight"), v_inverse_vp_light.size(), false, &v_inverse_vp_light[0][0][0]));
    GL_CHECK(glUniform4f(shader->uniformLocation("csmDistance"), csm_distances[0], csm_distances[1], csm_distances[2], csm_distances[3]));

    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse"), 1, false, &inverse(_viewMatrix)[0][0]));
    glm::vec3 light_pos_view_space = glm::mat4(_viewMatrix) * glm::vec4(light_pos_world[0], light_pos_world[1], light_pos_world[2], 1.f);
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosViewSpace"), light_pos_view_space.x, light_pos_view_space.y, light_pos_view_space.z));
    glm::vec3 l_color = light->_color;
    if (_gammaCorrectionEnabled) {
      l_color = glm::pow(l_color, glm::vec3(2.2f));
    }
    GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), l_color.x, l_color.y, l_color.z));
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowsEnabled"), _shadowsEnabled));
    GL_CHECK(glUniform1i(shader->uniformLocation("gammaCorrectionEnabled"), _gammaCorrectionEnabled));
    GL_CHECK(glUniform1f(shader->uniformLocation("nearPlane"), _zNear));
    GL_CHECK(glUniform1f(shader->uniformLocation("smBias"), _settings._smBias));

    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }

  void RenderingSystemOpenGL::lightingSpot(Entity * spot_light)
  {
    /*auto shader = _shaderProgramDeferredLightingSpot;
    shader->bind();

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _gBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 0));

    auto light = spot_light->getComponent<SpotLight>();
    auto transform = spot_light->getComponent<Transform>();
    auto light_pos_world = transform->getTranslation();

    glm::vec3 light_pos_view_space = _viewMatrix * glm::vec4(light_pos_world, 1.f);
    glm::vec3 light_target_view_space = _viewMatrix * glm::vec4(light->_target, 1.f);
    glm::vec3 light_dir_view_space = glm::normalize(light_target_view_space - light_pos_view_space);
    GL_CHECK(glUniform3f(shader->uniformLocation("lightDirViewSpace"), light_dir_view_space.x, light_dir_view_space.y, light_dir_view_space.z));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosViewSpace"), light_pos_view_space.x, light_pos_view_space.y, light_pos_view_space.z));
    GL_CHECK(glUniform1f(shader->uniformLocation("cosThetaP"), cosf(glm::radians(light->_penumbraDegrees))));
    GL_CHECK(glUniform1f(shader->uniformLocation("cosThetaU"), cosf(glm::radians(light->_umbraDegrees))));

    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _shadowMaps[spot_light]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowMapSampler"), 1));
    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    _frontBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerScene"), 2));
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowsEnabled"), _shadowsEnabled));
    glm::mat4 v, p;
    light->getViewProjectionMatrix(v, p, transform->getTranslation());
    auto v_inv_vp_light = p * v * inverse(_viewMatrix);
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse_VP_light"), 1, false, &v_inv_vp_light[0][0]));

    glm::vec3 l_color = light->_color;
    if (_gammaCorrectionEnabled) {
      l_color = glm::pow(l_color, glm::vec3(2.2f));
    }
    GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), l_color.x, l_color.y, l_color.z));
    GL_CHECK(glUniform1i(shader->uniformLocation("gammaCorrectionEnabled"), _gammaCorrectionEnabled));

    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));*/
  }

  void RenderingSystemOpenGL::lightingPoint(Entity * point_light)
  {
    /*auto shader = _shaderProgramDeferredLightingPoint;
    shader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _gBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gBufferSampler"), 0));
    auto light = point_light->getComponent<PointLight>();
    auto light_pos_world = point_light->getComponent<Transform>()->getTranslation();
    glm::vec3 light_pos_view_space = _viewMatrix * glm::vec4(light_pos_world, 1.f);
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosViewSpace"), light_pos_view_space.x, light_pos_view_space.y, light_pos_view_space.z));
    glm::vec3 l_color = light->_color;
    if (_gammaCorrectionEnabled) {
      l_color = glm::pow(l_color, glm::vec3(2.2f));
    }
    GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), l_color.x, l_color.y, l_color.z));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _frontBuffer->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerScene"), 1));
    GL_CHECK(glUniform1i(shader->uniformLocation("gammaCorrectionEnabled"), _gammaCorrectionEnabled));

    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    _shadowMaps[point_light]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowSampler"), 2));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosWorldSpace"), light_pos_world.x, light_pos_world.y, light_pos_world.z));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse"), 1, false, &inverse(_viewMatrix)[0][0]));
    GL_CHECK(glUniform1f(shader->uniformLocation("near"), light->_zNear));
    GL_CHECK(glUniform1f(shader->uniformLocation("far"), light->_zFar));
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowsEnabled"), _shadowsEnabled));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));*/
  }

  void RenderingSystemOpenGL::lightingPass()
  {
#if PROFILE
    _timings[RenderStage::LIGHTING] = Timing();
#endif
    _lightingBuffer[0]->bind();
    _lightingBuffer[0]->setViewport();
    std::vector<float> color = { 0.f, 0.f, 0.f, 1.f };
    GL_CHECK(glClearBufferfv(GL_COLOR, 0, &color[0]));

    for (auto& l : _directionalLights) {
      lightingDirectional(l);
    }
    for (auto& l : _spotLights) {
      lightingSpot(l);
    }
    for (auto& l : _pointLights) {
      lightingPoint(l);
    }
#if PROFILE
    _timings[RenderStage::LIGHTING].stop();
#endif
  }

  void RenderingSystemOpenGL::godRayPostProcess()
  {
    for (auto& dl : _directionalLights) {
      auto l = dl->getComponent<DirectionalLight>();
      auto light_pos_view_space = glm::mat4(_viewMatrix) * glm::vec4(l->_pos[0], l->_pos[1], l->_pos[2], 1.f);
      glm::vec4 light_pos_screen_space = _projectionMatrix * light_pos_view_space;
      light_pos_screen_space /= light_pos_screen_space[3];
      light_pos_screen_space = light_pos_screen_space * 0.5f + 0.5f;

      _renderGodRays = _godRayEffect->render(light_pos_screen_space, light_pos_view_space, this);

      /* if (renderGodRaysScreenSpace(transform->getTranslation())) {
         pingPongFilter(2, _framebufferQuarterSize, _gaussKernel, _gaussKernel);

         GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebufferQuarterSize[0]->id()));
         GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _upsampleFramebuffers[0]->id()));
         GL_CHECK(glBlitFramebuffer(0, 0, _framebufferQuarterSize[0]->width(), _framebufferQuarterSize[0]->height(),
           0, 0, _upsampleFramebuffers[0]->width(), _upsampleFramebuffers[0]->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));*/

           /*_backBuffer->bind();
           _backBuffer->setViewport();

           auto shader = _shaderProgramLightGather;
           shader->bind();

           auto light_color = light->_color;
           if (_gammaCorrectionEnabled) {
             light_color = glm::pow(light_color, glm::vec3(2.2f));
           }
           GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), light_color.r, light_color.g, light_color.b));
           GL_CHECK(glUniform1f(shader->uniformLocation("weight"), _godRayWeight));

           GL_CHECK(glActiveTexture(GL_TEXTURE0));
           _frontBuffer->textures()[0]->bind();
           GL_CHECK(glUniform1i(shader->uniformLocation("sceneSampler"), 0));

           GL_CHECK(glActiveTexture(GL_TEXTURE1));
           _upsampleFramebuffers[0]->textures()[0]->bind();
           GL_CHECK(glUniform1i(shader->uniformLocation("lightSampler"), 1));

           GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

           std::swap(_frontBuffer, _backBuffer);
         }*/
    }
  }

  void RenderingSystemOpenGL::volumeLightPostProcess()
  {
    /*  for (auto& light : _lights) {
        auto dl = light->getComponent<DirectionalLight>();
        auto sl = light->getComponent<SpotLight>();
        auto pl = light->getComponent<PointLight>();

        _framebufferQuarterSize[0]->bind();
        _framebufferQuarterSize[0]->setViewport();

        glm::vec3 light_color;
        if (dl != nullptr) {
          renderLightVolumeDirectional(light);
          light_color = dl->_color;
        }
        else if (pl != nullptr) {
          renderLightVolumePoint(light);
          light_color = pl->_color;
        }
        else {
          if (!renderLightVolumeSpot(light)) {
            continue;
          }
          light_color = sl->_color;
        }

        auto weight_texture = _lightVolumeBilateralBlur ? _gradientBufferQuarterSize->textures()[0] : nullptr;
        pingPongFilter(2, _framebufferQuarterSize, _gaussKernel, _gaussKernel, weight_texture);

        GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebufferQuarterSize[0]->id()));
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _upsampleFramebuffers[0]->id()));
        GL_CHECK(glBlitFramebuffer(0, 0, _framebufferQuarterSize[0]->width(), _framebufferQuarterSize[0]->height(),
          0, 0, _upsampleFramebuffers[0]->width(), _upsampleFramebuffers[0]->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));

        _backBuffer->bind();
        _backBuffer->setViewport();

        auto shader = _shaderProgramLightGather;
        shader->bind();

        if (_gammaCorrectionEnabled) {
          light_color = glm::pow(light_color, glm::vec3(2.2f));
        }
        GL_CHECK(glUniform3f(shader->uniformLocation("lightColor"), light_color.r, light_color.g, light_color.b));
        GL_CHECK(glUniform1f(shader->uniformLocation("weight"), _volumeLightWeight));

        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        _frontBuffer->textures()[0]->bind();
        GL_CHECK(glUniform1i(shader->uniformLocation("sceneSampler"), 0));

        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        _upsampleFramebuffers[0]->textures()[0]->bind();
        GL_CHECK(glUniform1i(shader->uniformLocation("lightSampler"), 1));

        GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

        std::swap(_frontBuffer, _backBuffer);
      }*/
  }

  void RenderingSystemOpenGL::renderLightVolumeDirectional(Entity * light)
  {
    /* auto dl = light->getComponent<DirectionalLight>();
     auto transform = light->getComponent<Transform>();
     auto light_pos_world = transform->getTranslation();
     auto shader = _shaderProgramVolumeLight;
     shader->bind();
     std::vector<glm::mat4> v_inverse_vp_light;
     std::vector<glm::mat4> vp;
     auto shadow_maps = _shadowMaps[light];
     dl->getViewProjectionMatrices(_viewportSize, _zNear, _fov, _viewMatrix, light_pos_world, shadow_maps->width(), vp);
     for (unsigned int i = 0; i < vp.size(); i++) {
       v_inverse_vp_light.push_back(vp[i] * inverse(_viewMatrix));
     }
     auto csm = dl->_csmDistances;
     GL_CHECK(glUniform4f(shader->uniformLocation("csmDistance"), csm[0], csm[1], csm[2], csm[3]));
     GL_CHECK(glActiveTexture(GL_TEXTURE0));
     shadow_maps->textures()[0]->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation("shadowMapSampler"), 0));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse_VP_light"), v_inverse_vp_light.size(), false, &v_inverse_vp_light[0][0][0]));
     GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P_inverse"), 1, false, &inverse(_projectionMatrix)[0][0]));
     auto point = _projectionMatrix * glm::vec4(0.f, 0.f, -csm[3], 1.f);
     point /= point.w;
     point = point * 0.5f + 0.5f;
     GL_CHECK(glUniform1f(shader->uniformLocation("maxDepth"), point.z));
     GL_CHECK(glActiveTexture(GL_TEXTURE1));
     _sceneDepthbuffer->bind();
     GL_CHECK(glUniform1i(shader->uniformLocation("samplerSceneDepth"), 1));
     GL_CHECK(glUniform1f(shader->uniformLocation("numSamples"), _volumeLightSamples));
     int level = floor(log2(std::max(_frontBuffer->width(), _frontBuffer->height())));
     GL_CHECK(glUniform1i(shader->uniformLocation("level"), level));
     GL_CHECK(glUniform1f(shader->uniformLocation("near"), _zNear));
     GL_CHECK(glUniform1f(shader->uniformLocation("far"), _zFar));
     GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));*/
  }

  bool RenderingSystemOpenGL::renderLightVolumeSpot(Entity * light)
  {
    auto sl = light->getComponent<SpotLight>();
    auto light_pos_world = sl->_pos;

    glm::mat4 v, p;
    sl->getViewProjectionMatrix(v, p);
    //glm::mat4 vp = p * v;

   /* if (_frustumCullingEnabled) {
      auto vp_inverse = inverse(vp);
      std::array<glm::vec3, 8> cube_ndc;
      cube_ndc[0] = glm::vec3(-1.f, -1.f, -1.f);
      cube_ndc[1] = glm::vec3(1.f, -1.f, -1.f);
      cube_ndc[2] = glm::vec3(-1.f, 1.f, -1.f);
      cube_ndc[3] = glm::vec3(-1.f, -1.f, 1.f);
      cube_ndc[4] = glm::vec3(1.f, 1.f, -1.f);
      cube_ndc[5] = glm::vec3(-1.f, 1.f, 1.f);
      cube_ndc[6] = glm::vec3(1.f, -1.f, 1.f);
      cube_ndc[7] = glm::vec3(1.f, 1.f, 1.f);
      std::array<glm::vec3, 8> cube_world;
      for (unsigned int i = 0; i < 8; i++) {
        auto pos_world = vp_inverse * glm::vec4(cube_ndc[i], 1.f);
        pos_world /= pos_world.w;
        cube_world[i] = glm::vec3(pos_world);
      }
      if (isCulled(cube_world, _VP)) {
        return false;
      }
    }*/

    auto v_inverse_vp_light = p * v * glm::mat4(inverse(_viewMatrix));

    auto shader = _shaderProgramVolumeLightSpot;
    shader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _shadowMaps[light]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowMapSampler"), 0));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V_inverse_VP_light"), 1, false, &v_inverse_vp_light[0][0]));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("P_inverse"), 1, false, &inverse(_projectionMatrix)[0][0]));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("V"), 1, false, &_viewMatrix[0][0]));

    float max_depth_view_space = -sl->_zFar * 1.5f;
    auto point = _projectionMatrix * Vec4f({ 0.f, 0.f, max_depth_view_space, 1.f });
    point /= point[3];
    point = point * 0.5f + 0.5f;
    GL_CHECK(glUniform1f(shader->uniformLocation("maxDepth"), point[2]));

    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _sceneDepthbuffer->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerSceneDepth"), 1));
    GL_CHECK(glUniform1f(shader->uniformLocation("numSamples"), _volumeLightSamples));

    glm::vec3 light_pos_view_space = glm::mat4(_viewMatrix) * glm::vec4(light_pos_world[0], light_pos_world[1], light_pos_world[2], 1.f);
    glm::vec3 light_target_view_space = glm::mat4(_viewMatrix) * glm::vec4(glm::vec3(sl->_target), 1.f);
    glm::vec3 light_dir_view_space = glm::normalize(light_target_view_space - light_pos_view_space);
    GL_CHECK(glUniform3f(shader->uniformLocation("lightDirViewSpace"), light_dir_view_space.x, light_dir_view_space.y, light_dir_view_space.z));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosViewSpace"), light_pos_view_space.x, light_pos_view_space.y, light_pos_view_space.z));
    GL_CHECK(glUniform1f(shader->uniformLocation("thetaP"), glm::radians(sl->_penumbraDegrees)));
    GL_CHECK(glUniform1f(shader->uniformLocation("thetaU"), glm::radians(sl->_umbraDegrees)));
    GL_CHECK(glUniform1f(shader->uniformLocation("cosThetaP"), cosf(glm::radians(sl->_penumbraDegrees))));
    GL_CHECK(glUniform1f(shader->uniformLocation("cosThetaU"), cosf(glm::radians(sl->_umbraDegrees))));

    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    return true;
  }

  void RenderingSystemOpenGL::renderLightVolumePoint(Entity* light)
  {
    auto pl = light->getComponent<PointLight>();
    auto light_pos_world = pl->_pos;

    auto shader = _shaderProgramVolumeLightPoint;
    shader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _shadowMaps[light]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("shadowSampler"), 0));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _sceneDepthbuffer->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerSceneDepth"), 1));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP_inverse"), 1, false, &inverse(Mat4f(_projectionMatrix) * _viewMatrix)[0][0]));
    float max_depth_view_space = -pl->_zFar * 1.5f;
    auto point = _projectionMatrix * Vec4f({ 0.f, 0.f, max_depth_view_space, 1.f });
    point /= point[3];
    point = point * 0.5f + 0.5f;
    GL_CHECK(glUniform1f(shader->uniformLocation("maxDepth"), point[2]));
    GL_CHECK(glUniform1f(shader->uniformLocation("numSamples"), _volumeLightSamples));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosWorldSpace"), light_pos_world[0], light_pos_world[1], light_pos_world[2]));
    GL_CHECK(glUniform1f(shader->uniformLocation("near"), pl->_zNear));
    GL_CHECK(glUniform1f(shader->uniformLocation("far"), pl->_zFar));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }

  void RenderingSystemOpenGL::bloomPostProcess()
  {
#if PROFILE
    _timings[RenderStage::BLOOM] = Timing();
#endif

    auto tex = _lightingBuffer[0]->textures()[0];
    tex->bind();
    tex->setMinificationFilter(GL_LINEAR);

    for (unsigned int i = 0; i < _bloomStages.size(); i++) {
      if (i == 0) {
        _bloomStages[i]._fb[0]->bind();
        _bloomStages[i]._fb[0]->setViewport();
        auto shader = _shaderProgramBrightness;
        shader->bind();
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        tex->bind();
        GL_CHECK(glUniform1i(shader->uniformLocation("lightingSampler"), 0));
        GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        pingPongFilter(_bloomBlurSteps * 2, _bloomStages[i]._fb, _gaussKernel, _gaussKernel);
      }
      else {
        pingPongFilter(_bloomBlurSteps * 2, _bloomStages[i]._fb, _gaussKernel, _gaussKernel, nullptr, _bloomStages[i - 1]._fb[0]->textures()[0]);
      }
      _bloomStages[i].upsample();
    }
    tex->bind();
    tex->setMinificationFilter(GL_NEAREST);
#if PROFILE
    _timings[RenderStage::BLOOM].stop();
#endif
  }

  void RenderingSystemOpenGL::eyeAdaptionPostProcess()
  {
    _lightingBuffer[0]->textures()[0]->bind();
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
    int level = std::floor(std::log2(std::max(_lightingBuffer[0]->width(), _lightingBuffer[0]->height()))); // the lowest mip map level contains the average scene color
    _exposureBuffer[0]->bind();
    _exposureBuffer[0]->setViewport();
    auto shader = _shaderProgramExposure;
    shader->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("level"), level));
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _lightingBuffer[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("samplerScene"), 0));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _exposureBuffer[1]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("exposureOld"), 1));
    GL_CHECK(glUniform1f(shader->uniformLocation("alpha"), _lerpAlpha));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }

  void RenderingSystemOpenGL::depthOfFieldPostProcess()
  {
    auto tex = _lightingBuffer[0]->textures()[0];
    tex->bind();
    tex->setMinificationFilter(GL_LINEAR);
    pingPongFilter(2, _DOFBlurBuffer, _gaussKernel, _gaussKernel, nullptr, tex);
    tex->bind();
    tex->setMinificationFilter(GL_NEAREST);
  }

  void RenderingSystemOpenGL::computeGradient()
  {
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, _gBuffer->id()));
    GL_CHECK(glReadBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _gradientBufferXQuarterSize[0]->id()));
    GL_CHECK(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glBlitFramebuffer(0, 0, _gBuffer->width(), _gBuffer->height(), 0, 0, _gradientBufferXQuarterSize[0]->width(), _gradientBufferXQuarterSize[0]->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));

    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, _gradientBufferXQuarterSize[0]->id()));
    GL_CHECK(glReadBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _gradientBufferYQuarterSize[0]->id()));
    GL_CHECK(glDrawBuffer(GL_COLOR_ATTACHMENT0));
    GL_CHECK(glBlitFramebuffer(0, 0, _gradientBufferXQuarterSize[0]->width(), _gradientBufferXQuarterSize[0]->height(), 0, 0,
      _gradientBufferYQuarterSize[0]->width(), _gradientBufferYQuarterSize[0]->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));

    pingPongFilter(2, _gradientBufferXQuarterSize, _sobelKernelSmooth, _sobelKernelGradient); // compute gradient in x direction
    pingPongFilter(2, _gradientBufferYQuarterSize, _sobelKernelGradient, _sobelKernelSmooth); // compute gradient in y direction

    _gradientBufferQuarterSize->bind();
    _gradientBufferQuarterSize->setViewport();
    _shaderProgramGradient->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    _gradientBufferXQuarterSize[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(_shaderProgramGradient->uniformLocation("samplerGradX"), 0));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    _gradientBufferYQuarterSize[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(_shaderProgramGradient->uniformLocation("samplerGradY"), 1));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }

  void RenderingSystemOpenGL::pingPongFilter(unsigned int steps, const std::array<std::shared_ptr<GLFramebufferOld>, 2>& fb,
    const std::vector<float>& kernel_horizontal, const std::vector<float>& kernel_vertical, const std::shared_ptr<GLTextureOld>& weight_texture, const std::shared_ptr<GLTextureOld>& base_texture)
  {
    auto shader = weight_texture != nullptr ? _shaderProgramPingPongFilterBilateral : _shaderProgramPingPongFilter;
    shader->bind();

    bool horizontal = true;
    bool first_time = true;
    for (unsigned int i = 0; i < steps; i++) {
      fb[horizontal]->bind();
      fb[horizontal]->setViewport();
      auto kernel = horizontal ? kernel_horizontal : kernel_vertical;
      GL_CHECK(glUniform1fv(shader->uniformLocation("kernel"), kernel.size(), &kernel[0]));
      glm::vec2 texel_size = 1.f / glm::vec2(fb[horizontal]->width(), fb[horizontal]->height()) * glm::vec2(horizontal, !horizontal);
      GL_CHECK(glUniform2f(shader->uniformLocation("texelSize"), texel_size.x, texel_size.y));
      GL_CHECK(glUniform1i(shader->uniformLocation("kernelSize"), kernel.size()));
      GL_CHECK(glUniform1i(shader->uniformLocation("kernelSizeHalf"), kernel.size() * 0.5f));
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      if (first_time && base_texture) {
        base_texture->bind();
      }
      else {
        fb[!horizontal]->textures()[0]->bind();
      }
      GL_CHECK(glUniform1i(shader->uniformLocation("sampler"), 0));
      if (weight_texture != nullptr) {
        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        weight_texture->bind();
        GL_CHECK(glUniform1i(shader->uniformLocation("samplerWeight"), 1));
      }
      GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
      horizontal = !horizontal;
      first_time = false;
    }
  }

  void RenderingSystemOpenGL::renderSkybox()
  {
    if (_skyboxTexture) {
      std::vector<GLenum> draw_buffers = { GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
      GL_CHECK(glDrawBuffers(draw_buffers.size(), &draw_buffers.front()));
      auto shader = _shaderProgramSkybox;
      shader->bind();
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      _skyboxTexture->bind();
      GL_CHECK(glUniform1i(shader->uniformLocation("skyboxSampler"), 0));
      Mat4f view_matrix = glm::mat4(glm::mat3(_viewMatrix)); // removes the translation part of the view matrix
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), 1, false, &(_projectionMatrix * view_matrix)[0][0]));
      _skyboxVertexArray->bind();
      _skyboxIndexbuffer->bind();
      GL_CHECK(glDrawElements(GL_TRIANGLES, _skyBoxIndices.size(), GL_UNSIGNED_INT, nullptr));
      draw_buffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
      GL_CHECK(glDrawBuffers(draw_buffers.size(), &draw_buffers.front()));
    }
  }

  void RenderingSystemOpenGL::renderSkydome()
  {
    if (_skydomeMesh) {
      std::vector<GLenum> draw_buffers = { GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
      GL_CHECK(glDrawBuffers(draw_buffers.size(), &draw_buffers.front()));
      GL_CHECK(glDisable(GL_CULL_FACE));
      auto shader = _skydomeShader;
      shader->bind();
      Mat4f view_matrix = glm::mat4(glm::mat3(_viewMatrix)); // removes the translation part of the view matrix
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      bindTextureOrLoadAsync("assets/clouds.png");
      GL_CHECK(glUniform1i(shader->uniformLocation("cloudSampler"), 0));
      GL_CHECK(glUniform1f(shader->uniformLocation("time"), _time));
     // auto wind_dir = _terrainRenderables.begin()->second->_terrain->getWindParams()._dir;
     // GL_CHECK(glUniform2f(shader->uniformLocation("windDir"), wind_dir.x, wind_dir.y));
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), 1, false, &(_projectionMatrix * view_matrix)[0][0]));
      GL_CHECK(glUniform3f(shader->uniformLocation("color"), _backGroundColor.r, _backGroundColor.g, _backGroundColor.b));
      setupMeshBindings(_skydomeMesh);
      GL_CHECK(glDrawElements(GL_TRIANGLES, _skydomeMesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
      GL_CHECK(glEnable(GL_CULL_FACE));
      draw_buffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5 };
      GL_CHECK(glDrawBuffers(draw_buffers.size(), &draw_buffers.front()));
    }
  }

  void RenderingSystemOpenGL::renderShadowMapsDirectional(Entity* entity)
  {
    auto fb = _shadowMaps[entity];
    fb->bind();
    fb->setViewport();
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));

#if PROFILE
    _timings[RenderStage::TREES_SHADOW_MAP] = Timing();
#endif
    renderTrees(entity);
#if PROFILE
    _timings[RenderStage::TREES_SHADOW_MAP].stop();
#endif

    auto shader = _shaderProgramDepthLayered;
    shader->bind();

    std::vector<Mat4f> vp;
    auto dl = entity->getComponent<DirectionalLight>();
    dl->getViewProjectionMatrices(_aspectRatio, _zNear, _fov, inverse(_viewMatrix), dl->getViewMatrix(),_shadowMaps[entity]->width(), _settings._smFrustumSplits, vp);
    GL_CHECK(glUniform1i(shader->uniformLocation("numLayers"), vp.size()));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), vp.size(), false, &vp[0][0][0]));

    for (auto& m : _models) {
      if (m == entity) { // Don't render the light itself to the shadow map
        continue;
      }
      auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
      GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("M"), 1, false, &model_matrix[0][0]));
      auto model = m->getComponent<Model>();
      auto& meshes = model->getMeshes();
      auto& materials = model->getMaterials();
      unsigned int material_index = materials.size();
      for (auto& mesh : meshes) {
        // if (isFarAway(mesh, model_matrix)) {
        //   continue;
       //  }
        unsigned int material_index_new = mesh->getMaterialIndex();
        if (material_index != material_index_new) {
          auto& opacity_path = materials[material_index_new]->getOpacityPath();
          if (opacity_path != "") {
            GL_CHECK(glActiveTexture(GL_TEXTURE0));
            bindTextureOrLoadAsync(opacity_path);
            GL_CHECK(glUniform1i(shader->uniformLocation("samplerOpacity"), 0));
          }
          GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), opacity_path != "" && _textures[opacity_path] != nullptr));
        }
        setupMeshBindings(mesh);
        GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
        material_index = material_index_new;
      }
    }
  }

  void RenderingSystemOpenGL::renderShadowMapsSpot(Entity * entity)
  {
    auto fb = _shadowMaps[entity];
    fb->bind();
    fb->setViewport();
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));

    auto shader = _shaderProgramDepth;
    shader->bind();

    auto sl = entity->getComponent<SpotLight>();
    glm::mat4 v, p;
    sl->getViewProjectionMatrix(v, p);
    Mat4f vp = p * v;

    for (auto& m : _models) {
      if (m != entity) { // Don't render the light itself to the shadow map
        auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
        auto mvp = vp * model_matrix;
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
        auto model = m->getComponent<Model>();
        auto& meshes = model->getMeshes();
        auto& materials = model->getMaterials();
        unsigned int material_index = materials.size();
        for (auto& mesh : meshes) {
          //  if (isFarAway(mesh, model_matrix) || _frustumCullingEnabled && isCulled(mesh->getAABBVertices(), mvp)) {
           //   continue;
         //   }
          auto material_index_new = mesh->getMaterialIndex();
          if (material_index != material_index_new) {
            auto& opacity_path = materials[material_index_new]->getOpacityPath();
            if (opacity_path != "") {
              GL_CHECK(glActiveTexture(GL_TEXTURE0));
              bindTextureOrLoadAsync(opacity_path);
              GL_CHECK(glUniform1i(shader->uniformLocation("samplerOpacity"), 0));
            }
            GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), opacity_path != "" && _textures[opacity_path] != nullptr));
          }
          setupMeshBindings(mesh);
          GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
          material_index = material_index_new;
        }
      }
    }
  }

  void RenderingSystemOpenGL::renderShadowMapsPoint(Entity* entity)
  {
    auto fb = _shadowMaps[entity];
    fb->bind();
    fb->setViewport();
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));

    auto shader = _shaderProgramDepthPointLight;
    shader->bind();

    auto pl = entity->getComponent<PointLight>();
    std::vector<glm::mat4> vp;
    pl->getViewProjectionMatrices(vp);
    GL_CHECK(glUniform1i(shader->uniformLocation("numLayers"), vp.size()));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosWorld"), pl->_pos[0], pl->_pos[1], pl->_pos[2]));
    GL_CHECK(glUniform1f(shader->uniformLocation("near"), pl->_zNear));
    GL_CHECK(glUniform1f(shader->uniformLocation("far"), pl->_zFar));
    GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("VP"), vp.size(), false, &vp[0][0][0]));

    for (auto& m : _models) {
      if (m != entity) { // Don't render the light itself to the shadow map
        auto model_matrix = m->getComponent<Transform>()->getModelMatrix();
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("M"), 1, false, &model_matrix[0][0]));
        auto model = m->getComponent<Model>();
        auto& materials = model->getMaterials();
        unsigned int material_index = materials.size();
        for (auto& mesh : model->getMeshes()) {
          //   if (isFarAway(mesh, model_matrix)) {
           //    continue;
           //  }
          unsigned int material_index_new = mesh->getMaterialIndex();
          if (material_index != material_index_new) {
            auto& opacity_path = materials[material_index_new]->getOpacityPath();
            if (opacity_path != "") {
              GL_CHECK(glActiveTexture(GL_TEXTURE0));
              bindTextureOrLoadAsync(opacity_path);
              GL_CHECK(glUniform1i(shader->uniformLocation("samplerOpacity"), 0));
            }
            GL_CHECK(glUniform1i(shader->uniformLocation("opacityEnabled"), opacity_path != "" && _textures[opacity_path] != nullptr));
          }
          setupMeshBindings(mesh);
          GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
          material_index = material_index_new;
        }
      }
    }
  }

  void RenderingSystemOpenGL::processTextureFutures()
  {
    if (_textureFutures.size()) {
      auto f = *_textureFutures.begin();
      if (f.second.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        auto result = f.second.get();
        if (result._data) {
          _textures[f.first] = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
          _textures[f.first]->create();
          // auto tex_id = SOIL_create_OGL_texture(result._data, result._width, result._height, result._channels, _textures[f.first]->id(), SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
          _textures[f.first]->bind();
          _textures[f.first]->setMagnificationFilter(GL_LINEAR);
          _textures[f.first]->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
          auto format = result._channels == 4 ? GL_RGBA : GL_RGB;
          GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
          _textures[f.first]->setData(result._width, result._height, format, format, GL_UNSIGNED_BYTE, result._data);
          GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
          free(result._data);
        }
        _textureFutures.erase(f.first);
      }
    }
  }

  void RenderingSystemOpenGL::bindTextureOrLoadAsync(const std::string & path)
  {
    if (_textures[path] == nullptr) {
      std::ifstream is(path);
      if (is.good() && !_textureFutures.count(path)) {
        _textureFutures[path] = std::async(std::launch::async, [path, this]() {
          AsyncTextureResult result;
          result._data = SOIL_load_image(path.c_str(), &result._width, &result._height, &result._channels, SOIL_LOAD_AUTO);
          return result;
        });
      }
    }
    else {
      _textures[path]->bind();
    }
  }

  void RenderingSystemOpenGL::setupMeshBindings(const std::shared_ptr<Mesh>&  mesh)
  {
    if (!_meshBindings.count(mesh)) {
      _meshBindings[mesh] = std::make_shared<MeshBinding>(mesh);
    }
    _meshBindings[mesh]->_vertexArray->bind();
    _meshBindings[mesh]->_indexBuffer->bind();
  }

  RenderingSystemOpenGL::MeshBinding::MeshBinding(const std::shared_ptr<Mesh>& mesh)
  {
    _vertexArray = std::make_shared<GLVertexArrayOld>();
    _vertexArray->create();
    _vertexArray->bind();

    _vertexBuffer = std::make_shared<GLBufferOld>();
    _vertexBuffer->create();
    _vertexBuffer->bind();
    auto& vertices = mesh->getVertices();
    _vertexBuffer->setData(&vertices[0], vertices.size() * sizeof(vertices[0]));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _position))));

    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _normal))));

    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _uv))));

    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _tangent))));

    GL_CHECK(glEnableVertexAttribArray(4));
    GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _bitangent))));

    _indexBuffer = std::make_shared<GLBufferOld>();
    _indexBuffer->create(GL_ELEMENT_ARRAY_BUFFER);
    _indexBuffer->bind();
    auto& indices = mesh->getIndices();
    _indexBuffer->setData(&indices[0], indices.size() * sizeof(indices[0]));
  }

  RenderingSystemOpenGL::GrassQuadTree::GrassQuadTree(int size, int min_size, float error_threshold) : _size(size), _minSize(min_size), _errorThreshold(error_threshold)
  {
    auto root_node = new Node(glm::ivec2(0), _size);
    build(root_node);

    glm::vec2 center(size / 2);
    std::sort(_nodes.begin(), _nodes.end(), [center](Node* n1, Node* n2) {
      float dist1 = glm::distance(center, glm::vec2(n1->_pos));
      float dist2 = glm::distance(center, glm::vec2(n2->_pos));
      return dist1 < dist2;
    });
  }

  RenderingSystemOpenGL::GrassQuadTree::~GrassQuadTree()
  {
    for (auto& n : _nodes) {
      delete n;
    }
  }

  std::vector<RenderingSystemOpenGL::GrassQuadTree::Node*>& RenderingSystemOpenGL::GrassQuadTree::getNodes()
  {
    return _nodes;
  }

  int RenderingSystemOpenGL::GrassQuadTree::getSize()
  {
    return _size;
  }

  void RenderingSystemOpenGL::GrassQuadTree::build(Node* node)
  {
    glm::vec2 cam_pos_quadtree(_size * 0.5f);
    float error = node->_size / glm::distance(cam_pos_quadtree, node->center());
    int new_size = node->_size / 2;
    if (error > _errorThreshold && new_size >= _minSize) {
      auto south_west = new Node(node->_pos, new_size);
      auto south_east = new Node(node->_pos + glm::ivec2(new_size, 0), new_size);
      auto north_west = new Node(node->_pos + glm::ivec2(0, new_size), new_size);
      auto north_east = new Node(node->_pos + glm::ivec2(new_size), new_size);
      delete node;
      build(south_west);
      build(south_east);
      build(north_west);
      build(north_east);
    }
    else {
      _nodes.push_back(node);
    }
  }

  RenderingSystemOpenGL::GrassQuadTree::Node::Node(const glm::ivec2& pos, int size) : _pos(pos), _size(size)
  {
  }

  glm::vec2 RenderingSystemOpenGL::GrassQuadTree::Node::center()
  {
    return glm::vec2(_pos) + _size * 0.5f;
  }

  RenderingSystemOpenGL::BloomStage::BloomStage(const glm::ivec2& view_port_size, unsigned int level) : _level(level)
  {
    auto size = view_port_size / static_cast<int>(pow(2, level + 1));
    for (int i = 0; i < 2; i++) {
      _fb[i] = std::make_shared<GLFramebufferOld>();
      _fb[i]->create(size.x, size.y);
      _fb[i]->bind();
      auto tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      tex->create();
      tex->bind();
      tex->setMinificationFilter(GL_LINEAR);
      tex->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      tex->setData(size.x, size.y, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
      _fb[i]->addTexture(tex, GL_COLOR_ATTACHMENT0);
      _fb[i]->setDrawbuffersFromColorAttachments();
      _fb[i]->checkStatus();
    }

    if (level > 0) {
      size = view_port_size / 2;
      for (int i = 0; i <= level; i++, size /= 2) {
        _upsampleFb.push_back(std::make_shared<GLFramebufferOld>());
        auto& fb = _upsampleFb.back();
        fb->create(size.x, size.y);
        fb->bind();
        auto tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
        tex->create();
        tex->bind();
        tex->setMinificationFilter(GL_LINEAR);
        tex->setMagnificationFilter(GL_LINEAR);
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        tex->setData(size.x, size.y, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
        fb->addTexture(tex, GL_COLOR_ATTACHMENT0);
        fb->setDrawbuffersFromColorAttachments();
        fb->checkStatus();
      }
    }
  }
  RenderingSystemOpenGL::BloomStage::~BloomStage()
  {

  }

  void RenderingSystemOpenGL::BloomStage::upsample()
  {
    if (_level == 0) {
      _result = _fb[0]->textures()[0];
      return;
    }

    for (int i = _upsampleFb.size() - 1; i >= 0; i--) {
      auto fb_base = i == _upsampleFb.size() - 1 ? _fb[0] : _upsampleFb[i + 1];
      GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_base->id()));
      GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _upsampleFb[i]->id()));
      GL_CHECK(glBlitFramebuffer(0, 0, fb_base->width(), fb_base->height(), 0, 0, _upsampleFb[i]->width(), _upsampleFb[i]->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));
    }
    _result = _upsampleFb[0]->textures()[0];
  }

  RenderingSystemOpenGL::GodRayEffect::GodRayEffect(const glm::ivec2& view_port_size)
  {
    auto size = view_port_size / 4;
    for (auto& fb : _fb) {
      fb = std::make_shared<GLFramebufferOld>();
      fb->create(size.x, size.y);
      fb->bind();
      auto tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      tex->create();
      tex->bind();
      tex->setMinificationFilter(GL_LINEAR);
      tex->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      tex->setData(size.x, size.y, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
      fb->addTexture(tex, GL_COLOR_ATTACHMENT0);
      fb->setDrawbuffersFromColorAttachments();
      fb->checkStatus();
    }

    size = view_port_size / 2;
    _resultFb = std::make_shared<GLFramebufferOld>();
    _resultFb->create(size.x, size.y);
    _resultFb->bind();
    auto tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    tex->create();
    tex->bind();
    tex->setMinificationFilter(GL_LINEAR);
    tex->setMagnificationFilter(GL_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    tex->setData(size.x, size.y, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
    _resultFb->addTexture(tex, GL_COLOR_ATTACHMENT0);
    _resultFb->setDrawbuffersFromColorAttachments();
    _resultFb->checkStatus();
  }
  RenderingSystemOpenGL::GodRayEffect::~GodRayEffect()
  {

  }

  bool RenderingSystemOpenGL::GodRayEffect::render(const glm::vec3 & light_pos_screen, const glm::vec3& light_pos_view_space, RenderingSystemOpenGL* rs)
  {
    bool light_visible = light_pos_view_space.z < 0.f && glm::all(glm::greaterThan(light_pos_screen, glm::vec3(0.f))) && glm::all(glm::lessThan(glm::vec2(light_pos_screen), glm::vec2(1.f)));
    if (!light_visible) {
      return false;
    }
    float dist_to_border = std::min(light_pos_screen.x, std::min(1.f - light_pos_screen.x, std::min(light_pos_screen.y, 1.f - light_pos_screen.y)));
    float fade_dist = 0.1f;
    float fade = glm::clamp(dist_to_border / fade_dist, 0.f, 1.f);

    _fb[0]->bind();
    _fb[0]->setViewport();
    auto shader = rs->_shaderProgramGodRay;
    shader->bind();
    glActiveTexture(GL_TEXTURE0);
    rs->_lightingBuffer[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("lightingSampler"), 0));
    GL_CHECK(glUniform1f(shader->uniformLocation("numSamples"), rs->_godRaySamples));
    GL_CHECK(glUniform1f(shader->uniformLocation("fade"), fade));
    GL_CHECK(glUniform1f(shader->uniformLocation("decay"), rs->_godRayDecay));
    GL_CHECK(glUniform3f(shader->uniformLocation("lightPosScreen"), light_pos_screen.x, light_pos_screen.y, light_pos_screen.z));
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    rs->pingPongFilter(2, _fb, rs->_gaussKernel, rs->_gaussKernel);

    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, _fb[0]->id()));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _resultFb->id()));
    GL_CHECK(glBlitFramebuffer(0, 0, _fb[0]->width(), _fb[0]->height(),
      0, 0, _resultFb->width(), _resultFb->height(), GL_COLOR_BUFFER_BIT, GL_LINEAR));

    return true;
  }

  RenderingSystemOpenGL::TerrainRenderable::TerrainRenderable(Entity* terrain, RenderingSystemOpenGL* rs) : _terrain(terrain->getComponent<Terrain>()), _transform(terrain->getComponent<Transform>())
  {
    _terrainVao = std::make_shared<GLVertexArrayOld>();
    _terrainVao->create();
    _terrainVao->bind();
    GL_CHECK(glEnableVertexAttribArray(0));

    _terrainVbo = std::make_shared<GLBufferOld>();
    _terrainVbo->create();
    _terrainVbo->bind();
    auto vertices = _terrain->getTileVertices();
    _terrainVbo->setData(&vertices.front(), vertices.size() * sizeof(vertices.front()));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr));

    for (int lod = 0; lod <= _terrain->getMaxLOD(); lod++) {
      createIndexBuffer(lod, NONE, _terrain->getTileIndices(lod));
      createIndexBuffer(lod, EAST, _terrain->getTileIndicesEast(lod));
      createIndexBuffer(lod, WEST, _terrain->getTileIndicesWest(lod));
      createIndexBuffer(lod, NORTH, _terrain->getTileIndicesNorth(lod));
      createIndexBuffer(lod, SOUTH, _terrain->getTileIndicesSouth(lod));
      createIndexBuffer(lod, NORTH | EAST, _terrain->getTileIndicesNorthEast(lod));
      createIndexBuffer(lod, EAST | SOUTH, _terrain->getTileIndicesSouthEast(lod));
      createIndexBuffer(lod, SOUTH | WEST, _terrain->getTileIndicesSouthWest(lod));
      createIndexBuffer(lod, WEST | NORTH, _terrain->getTileIndicesNorthWest(lod));
      createIndexBuffer(lod, NORTH | SOUTH, _terrain->getTileIndicesNorthSouth(lod));
      createIndexBuffer(lod, EAST | WEST, _terrain->getTileIndicesEastWest(lod));
      createIndexBuffer(lod, NORTH | EAST | SOUTH, _terrain->getTileIndicesNorthEastSouth(lod));
      createIndexBuffer(lod, EAST | SOUTH | WEST, _terrain->gettileIndicesEastSouthWest(lod));
      createIndexBuffer(lod, SOUTH | WEST | NORTH, _terrain->getTileIndicesSouthWestNorth(lod));
      createIndexBuffer(lod, WEST | NORTH | EAST, _terrain->getTileIndicesWestNorthEast(lod));
      createIndexBuffer(lod, NORTH | EAST | SOUTH | WEST, _terrain->getTileIndicesNorthWestSouthEast(lod));
    }

    for (unsigned int i = 0; i < 2; i++) {
      _impostorFb[i] = std::make_shared<GLFramebufferOld>();
      _impostorFb[i]->create(256, 256);
      _impostorFb[i]->bind();
      auto tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      tex->create();
      tex->bind();
      tex->setMagnificationFilter(GL_LINEAR);
      tex->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
      tex->setData(_impostorFb[i]->width(), _impostorFb[i]->height(), GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
      _impostorFb[i]->addTexture(tex, GL_COLOR_ATTACHMENT0);
      auto impostor_depth_buffer = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      impostor_depth_buffer->create();
      impostor_depth_buffer->bind();
      impostor_depth_buffer->setMinificationFilter(GL_NEAREST);
      impostor_depth_buffer->setMagnificationFilter(GL_NEAREST);
      impostor_depth_buffer->setData(_impostorFb[i]->width(), _impostorFb[i]->height(), GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
      _impostorFb[i]->setDepthTexture(impostor_depth_buffer);
      _impostorFb[i]->checkStatus();
    }

    _heightMap = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    _heightMap->create();
    _heightMap->bind();
    _heightMap->setMagnificationFilter(GL_LINEAR);
    _heightMap->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    std::vector<GLint> swizzle_mask = { GL_RED, GL_RED, GL_RED, GL_RED };
    GL_CHECK(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swizzle_mask[0]));
    auto height_map = _terrain->getHeightMap();
    _heightMap->setData(height_map.cols, height_map.rows, GL_R16F, GL_RED, GL_FLOAT, height_map.data);
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    _splatMap = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    _splatMap->create();
    _splatMap->bind();
    _splatMap->setMagnificationFilter(GL_LINEAR);
    _splatMap->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
    //GL_CHECK(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swizzle_mask[0]));
    auto& splat_map = _terrain->getSplatMap();
    _splatMap->setData(splat_map.cols, splat_map.rows, GL_RGBA16F, GL_RGBA, GL_FLOAT, splat_map.data);
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    std::array<std::shared_ptr<GLFramebufferOld>, 2> fbs_grad_x;
    for (unsigned int i = 0; i < 2; i++) {
      auto grad_tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      grad_tex->create();
      grad_tex->bind();
      grad_tex->setMagnificationFilter(GL_LINEAR);
      grad_tex->setMinificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swizzle_mask[0]));
      grad_tex->setData(height_map.cols, height_map.rows, GL_R16F, GL_RED, GL_FLOAT, height_map.data);
      fbs_grad_x[i] = std::make_shared<GLFramebufferOld>();
      fbs_grad_x[i]->create(height_map.cols, height_map.rows);
      fbs_grad_x[i]->bind();
      fbs_grad_x[i]->addTexture(grad_tex, GL_COLOR_ATTACHMENT0);
      fbs_grad_x[i]->checkStatus();
    }

    std::array<std::shared_ptr<GLFramebufferOld>, 2> fbs_grad_y;
    for (unsigned int i = 0; i < 2; i++) {
      auto grad_tex = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      grad_tex->create();
      grad_tex->bind();
      grad_tex->setMagnificationFilter(GL_LINEAR);
      grad_tex->setMinificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swizzle_mask[0]));
      grad_tex->setData(height_map.cols, height_map.rows, GL_R16F, GL_RED, GL_FLOAT, height_map.data);
      fbs_grad_y[i] = std::make_shared<GLFramebufferOld>();
      fbs_grad_y[i]->create(height_map.cols, height_map.rows);
      fbs_grad_y[i]->bind();
      fbs_grad_y[i]->addTexture(grad_tex, GL_COLOR_ATTACHMENT0);
      fbs_grad_y[i]->checkStatus();
    }

    rs->pingPongFilter(2, fbs_grad_x, rs->_sobelKernelGradient, rs->_sobelKernelSmooth);
    rs->pingPongFilter(2, fbs_grad_y, rs->_sobelKernelSmooth, rs->_sobelKernelGradient);

    _normalMap = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    _normalMap->create();
    _normalMap->bind();
    _normalMap->setMagnificationFilter(GL_LINEAR);
    _normalMap->setMinificationFilter(GL_LINEAR_MIPMAP_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    _normalMap->setData(height_map.cols, height_map.rows, GL_RGB16F, GL_RED, GL_FLOAT, nullptr);
    auto normal_fb = std::make_shared<GLFramebufferOld>();
    normal_fb->create(height_map.cols, height_map.rows);
    normal_fb->bind();
    normal_fb->addTexture(_normalMap, GL_COLOR_ATTACHMENT0);
    normal_fb->checkStatus();

    normal_fb->setViewport();
    auto shader = rs->_normalMapShader;
    shader->bind();

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    fbs_grad_x[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gradXSampler"), 0));

    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    fbs_grad_y[0]->textures()[0]->bind();
    GL_CHECK(glUniform1i(shader->uniformLocation("gradYSampler"), 1));

    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    _normalMap->bind();
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    {
      _meshVbo = std::make_shared<GLBufferOld>();
      _meshVbo->create();
      _meshVbo->bind();
      auto& trunk_vertices_lod0 = _terrain->getTreeModelLod0()->getMeshes()[0]->getVertices();
      auto& trunk_vertices_lod1 = _terrain->getTreeModelLod1()->getMeshes()[0]->getVertices();
      auto& leaf_vertices = _terrain->getLeavesModel()->getMeshes()[0]->getVertices();
      std::vector<Vertex> vertices = trunk_vertices_lod0;
      vertices.insert(vertices.end(), trunk_vertices_lod1.begin(), trunk_vertices_lod1.end());
      vertices.insert(vertices.end(), leaf_vertices.begin(), leaf_vertices.end());
      _meshVbo->setData(&vertices.front(), vertices.size() * sizeof(vertices.front()));

      _meshIbo = std::make_shared<GLBufferOld>();
      _meshIbo->create(GL_ELEMENT_ARRAY_BUFFER);
      _meshIbo->bind();
      auto& trunk_indices_lod0 = _terrain->getTreeModelLod0()->getMeshes()[0]->getIndices();
      auto& trunk_indices_lod1 = _terrain->getTreeModelLod1()->getMeshes()[0]->getIndices();
      auto& leaf_indices = _terrain->getLeavesModel()->getMeshes()[0]->getIndices();
      std::vector<unsigned> indices = trunk_indices_lod0;
      indices.insert(indices.end(), trunk_indices_lod1.begin(), trunk_indices_lod1.end());
      indices.insert(indices.end(), leaf_indices.begin(), leaf_indices.end());
      _meshIbo->setData(&indices.front(), indices.size() * sizeof(indices.front()));

      _trunkLod1BaseVertex = trunk_vertices_lod0.size();
      _trunkLod1IdxOffset = reinterpret_cast<GLvoid*>(trunk_indices_lod0.size() * sizeof(trunk_indices_lod0.front()));

      _leafsBaseVert = trunk_vertices_lod0.size() + trunk_vertices_lod1.size();
      _leafsIdxOffs = reinterpret_cast<GLvoid*>((trunk_indices_lod0.size() + trunk_indices_lod1.size()) * sizeof(trunk_indices_lod0.front()));
    }

    std::vector<Terrain::TreeNode*> nodes;
    _terrain->getAllNodes(nodes);
    for (auto n : nodes) {
      auto& tree_transforms = n->_transforms;
      auto& scales = n->_scales;

      if (tree_transforms.size()) {
        _treeVao[n] = std::make_shared<GLVertexArrayOld>();
        _treeVao[n]->create();
        _treeVao[n]->bind();
        GL_CHECK(glEnableVertexAttribArray(0));
        GL_CHECK(glEnableVertexAttribArray(1));
        GL_CHECK(glEnableVertexAttribArray(2));
        GL_CHECK(glEnableVertexAttribArray(3));
        GL_CHECK(glEnableVertexAttribArray(4));
        GL_CHECK(glEnableVertexAttribArray(5));
        GL_CHECK(glEnableVertexAttribArray(6));
        GL_CHECK(glEnableVertexAttribArray(7));
        GL_CHECK(glEnableVertexAttribArray(8));
        GL_CHECK(glEnableVertexAttribArray(9));

        _meshVbo->bind();
        GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _position))));
        GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _normal))));
        GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _uv))));
        GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _tangent))));;
        GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, _bitangent))));

        _treeTransformVbo[n] = std::make_shared<GLBufferOld>();
        _treeTransformVbo[n]->create(GL_ARRAY_BUFFER);
        _treeTransformVbo[n]->bind();


        _treeTransformVbo[n]->setData(nullptr, tree_transforms.size() * sizeof(tree_transforms.front()) + scales.size() * sizeof(scales.front()));

        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, tree_transforms.size() * sizeof(tree_transforms.front()), &tree_transforms.front()[0][0]));
        GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, tree_transforms.size() * sizeof(tree_transforms.front()), scales.size() * sizeof(scales.front()), &scales.front()));

        int col_size = sizeof(tree_transforms.front()) / 4;
        GL_CHECK(glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(tree_transforms.front()), nullptr));
        GL_CHECK(glVertexAttribPointer(6, 4, GL_FLOAT, false, sizeof(tree_transforms.front()), reinterpret_cast<void*>(col_size)));
        GL_CHECK(glVertexAttribPointer(7, 4, GL_FLOAT, false, sizeof(tree_transforms.front()), reinterpret_cast<void*>(2 * col_size)));
        GL_CHECK(glVertexAttribPointer(8, 4, GL_FLOAT, false, sizeof(tree_transforms.front()), reinterpret_cast<void*>(3 * col_size)));
        GL_CHECK(glVertexAttribPointer(9, 1, GL_FLOAT, false, sizeof(scales.front()), reinterpret_cast<void*>(tree_transforms.size() * sizeof(tree_transforms.front()))));
        GL_CHECK(glVertexAttribDivisor(5, 1));
        GL_CHECK(glVertexAttribDivisor(6, 1));
        GL_CHECK(glVertexAttribDivisor(7, 1));
        GL_CHECK(glVertexAttribDivisor(8, 1));
        GL_CHECK(glVertexAttribDivisor(9, 1));
      }

      if (n->_cloudBillboardPositionsAndScales.size()) {
        _cloudBillboardsVao[n] = std::make_shared<GLVertexArrayOld>();
        _cloudBillboardsVao[n]->create();
        _cloudBillboardsVao[n]->bind();
        GL_CHECK(glEnableVertexAttribArray(0));

        _cloudBillboardsVbo[n] = std::make_shared<GLBufferOld>();
        _cloudBillboardsVbo[n]->create(GL_ARRAY_BUFFER);
        _cloudBillboardsVbo[n]->bind();
        auto& cloud_transforms = n->_cloudBillboardPositionsAndScales;
        _cloudBillboardsVbo[n]->setData(&cloud_transforms.front(), cloud_transforms.size() * sizeof(cloud_transforms.front()));
        GL_CHECK(glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0));
        GL_CHECK(glVertexAttribDivisor(0, 1));
      }
    }
  }

  void RenderingSystemOpenGL::TerrainRenderable::createIndexBuffer(int lod, int dir, const std::vector<unsigned>& indices)
  {
    _terrainIbo[lod][dir] = std::make_shared<GLBufferOld>();
    _terrainIbo[lod][dir]->create(GL_ELEMENT_ARRAY_BUFFER);
    _terrainIbo[lod][dir]->bind();
    _terrainIbo[lod][dir]->setData(&indices[0], indices.size() * sizeof(indices[0]));
    _terrainNumIndices[lod][dir] = indices.size();
  }

  void RenderingSystemOpenGL::TerrainRenderable::renderImpostor(const std::shared_ptr<Model>& tree_model, const std::shared_ptr<Model>& leaf_model, const glm::mat4 & transform, RenderingSystemOpenGL * rs)
  {
    std::swap(_impostorFb[0], _impostorFb[1]);

    GL_CHECK(glEnable(GL_DEPTH_TEST));

    _impostorFb[1]->bind();
    //   _impostorFb->setViewport();

    GL_CHECK(glClearBufferfv(GL_COLOR, 0, &std::vector<float>(4, 0.f)[0]));
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));

    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());
    auto model_view = glm::mat4(rs->_viewMatrix) * transform;
    for (auto& v : tree_model->getMeshes()[0]->getAABB()->getVertices()) {
      glm::vec3 v_view = model_view * glm::vec4(glm::vec3(v), 1.f);
      v_view.z *= -1.f;
      min = glm::min(min, v_view);
      max = glm::max(max, v_view);
    }
    for (auto& v : leaf_model->getMeshes()[0]->getAABB()->getVertices()) {
      glm::vec3 v_view = model_view * glm::vec4(glm::vec3(v), 1.f);
      v_view.z *= -1.f;
      min = glm::min(min, v_view);
      max = glm::max(max, v_view);
    }
    auto projection_matrix = glm::ortho(min.x, max.x, min.y, max.y, min.z, max.z);
    auto mvp = projection_matrix * model_view;

    for (int x = 0; x < 2; x++) {
      for (int y = 0; y < 2; y++) {
        auto shader = rs->_impostorShader;
        shader->bind();
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
        GL_CHECK(glViewport(x * _impostorFb[1]->width() / 2, y * _impostorFb[1]->height() / 2, _impostorFb[1]->width() / 2, _impostorFb[1]->height() / 2));
        for (auto& mesh : tree_model->getMeshes()) {
          GL_CHECK(glActiveTexture(GL_TEXTURE0));
          rs->bindTextureOrLoadAsync(tree_model->getMaterials()[mesh->getMaterialIndex()]->getDiffusePath());
          GL_CHECK(glUniform1i(shader->uniformLocation("diffuseSampler"), 0));
          rs->setupMeshBindings(mesh);
          GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
        }

        shader = rs->_leavesImpostorShader;
        shader->bind();
        GL_CHECK(glUniformMatrix4fv(shader->uniformLocation("MVP"), 1, false, &mvp[0][0]));
        GL_CHECK(glUniform2f(shader->uniformLocation("idx_2d"), x, y));
        GL_CHECK(glDisable(GL_CULL_FACE));
        for (auto& mesh : leaf_model->getMeshes()) {
          GL_CHECK(glActiveTexture(GL_TEXTURE0));
          rs->bindTextureOrLoadAsync(leaf_model->getMaterials()[mesh->getMaterialIndex()]->getDiffusePath());
          GL_CHECK(glUniform1i(shader->uniformLocation("diffuseSampler"), 0));
          rs->setupMeshBindings(mesh);
          GL_CHECK(glDrawElements(GL_TRIANGLES, mesh->getIndices().size(), GL_UNSIGNED_INT, nullptr));
        }
        GL_CHECK(glEnable(GL_CULL_FACE));
      }
    }
    _impostorFb[1]->textures()[0]->bind();
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
  }

  Mat4f RenderingSystemOpenGL::TerrainRenderable::getWaterModelMatrix()
  {
    return _transform->getModelMatrix() * scale<4, float>(Vec3f(static_cast<float>(_terrain->getHeightMap().cols))) * translate<4, float>(Vec3f( 0.f, 0.00015f, 0.f ));
  }

  RenderingSystemOpenGL::~RenderingSystemOpenGL()
  {
  }

  void RenderingSystemOpenGL::setSkybox(const std::array<std::string, 6u>& paths)
  {
    _skyboxTexture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    _skyboxTexture->create();
    SOIL_load_OGL_cubemap(paths[0].c_str(), paths[1].c_str(), paths[2].c_str(), paths[3].c_str(), paths[4].c_str(), paths[5].c_str(), SOIL_LOAD_AUTO, _skyboxTexture->id(), 0);

    /* _skyboxTexture = std::make_shared<GLTexture>(GL_TEXTURE_CUBE_MAP);
     _skyboxTexture->create();
     _skyboxTexture->bind();

     for (unsigned int i = 0; i < 6; i++) {
       GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, face_textures[i]->cols, face_textures[i]->rows, 0, GL_BGR, GL_UNSIGNED_BYTE, face_textures[i]->data));
     }
     _skyboxTexture->setMinificationFilter(GL_LINEAR);
     _skyboxTexture->setMagnificationFilter(GL_LINEAR);
     GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
     GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
     GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));*/
  }

  void RenderingSystemOpenGL::setSkydome(const std::shared_ptr<Mesh>& mesh)
  {
    _skydomeMesh = mesh;
  }

  void RenderingSystemOpenGL::initShaders()
  {
    createShader(_shaderProgramDepth, "assets/opengl/vs_depth.glsl", "assets/opengl/fs_depth.glsl");
    createShader(_impostorShader, "assets/opengl/vs_impostor.glsl", "assets/opengl/fs_impostor.glsl");
    createShader(_treeBillboardShader, "assets/opengl/vs_tree_billboard.glsl", "assets/opengl/fs_tree_billboard.glsl");
    createShader(_shaderProgramDepthLayered, "assets/opengl/vs_depth_layered.glsl", "assets/opengl/fs_depth.glsl", "assets/opengl/gs_depth_layered.glsl");
    createShader(_shaderProgramDepthPointLight, "assets/opengl/vs_depth_layered.glsl", "assets/opengl/fs_depth_point_light.glsl", "assets/opengl/gs_depth_layered.glsl");
    createShader(_shaderProgramScreen, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_screen.glsl");
    createShader(_shaderProgramGodRay, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_god_ray.glsl");
    createShader(_shaderProgramExposure, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_exposure.glsl");
    createShader(_shaderProgramDeferredGeometryTextured, "assets/opengl/vs_deferred_geometry_textured.glsl", "assets/opengl/fs_deferred_geometry_textured.glsl");
    createShader(_treeShader, "assets/opengl/vs_tree.glsl", "assets/opengl/fs_deferred_geometry_textured.glsl");
    createShader(_terrainShader, "assets/opengl/vs_terrain.glsl", "assets/opengl/fs_terrain.glsl");
    createShader(_shaderProgramDeferredLightingDirectional, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_deferred_lighting_directional.glsl");
    createShader(_shaderProgramDeferredLightingSpot, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_deferred_lighting_spot.glsl");
    createShader(_shaderProgramDeferredLightingPoint, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_deferred_lighting_point.glsl");
    createShader(_shaderProgramLightGather, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_light_gather.glsl");
    createShader(_shaderProgramUnderwater, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_underwater.glsl");
    createShader(_shaderProgramWaterPopupDistortion, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_water_popup_distortion.glsl");
    createShader(_shaderProgramPingPongFilter, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_ping_pong_filter.glsl");
    createShader(_shaderProgramPingPongFilterBilateral, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_ping_pong_filter_bilateral.glsl");
    createShader(_shaderProgramBloom, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_bloom.glsl");
    createShader(_shaderProgramMotionBlur, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_motion_blur.glsl");
    createShader(_shaderProgramBrightness, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_brightness.glsl");
    createShader(_shaderProgramVolumeLight, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_light_volume.glsl");
    createShader(_shaderProgramGradient, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_gradient.glsl");
    createShader(_shaderProgramVolumeLightSpot, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_light_volume_spot.glsl");
    createShader(_shaderProgramVolumeLightPoint, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_light_volume_point.glsl");
    createShader(_shaderProgramDOF, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_DOF.glsl");
    createShader(_shaderProgramDOFRefDepth, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_DOF_ref_depth.glsl");
    createShader(_shaderProgramSkybox, "assets/opengl/vs_skybox.glsl", "assets/opengl/fs_skybox.glsl");
    createShader(_shaderProgramWaterForward, "assets/opengl/vs_water_forward.glsl", "assets/opengl/fs_water_forward.glsl");
    //createShader(_aabbShader, "assets/opengl/vs_aabb.glsl", "assets/opengl/fs_aabb.glsl");
    createShader(_lensFlareShader, "assets/opengl/vs_lens_flare.glsl", "assets/opengl/fs_lens_flare.glsl");
    createShader(_lensFlareCombineShader, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_lens_flare_combine.glsl");
    createShader(_skydomeShader, "assets/opengl/vs_skybox.glsl", "assets/opengl/fs_skydome.glsl");
    createShader(_ssrShader, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_ssr.glsl");
    createShader(_terrainWireframeShader, "assets/opengl/vs_terrain_wireframe.glsl", "assets/opengl/fs_aabb.glsl");
    createShader(_normalMapShader, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_create_normal_map.glsl");
    createShader(_grassShader, "assets/opengl/vs_grass.glsl", "assets/opengl/fs_grass.glsl", "assets/opengl/gs_grass.glsl");
    createShader(_treeShadowMapShader, "assets/opengl/vs_tree_depth_layered.glsl", "assets/opengl/fs_tree_depth_layered.glsl", "assets/opengl/gs_tree_depth_layered.glsl");
    createShader(_leavesShader, "assets/opengl/vs_leaves.glsl", "assets/opengl/fs_leaves.glsl");
    createShader(_leavesShadowMapShader, "assets/opengl/vs_leaves_depth.glsl", "assets/opengl/fs_leaves_depth.glsl", "assets/opengl/gs_leaves_depth_layered.glsl");
    createShader(_leavesImpostorShader, "assets/opengl/vs_leaf_impostor.glsl", "assets/opengl/fs_leaf_impostor.glsl");
    createShader(_cloudBillboardShader, "assets/opengl/vs_cloud_billboard.glsl", "assets/opengl/fs_cloud_billboard.glsl");
    createShader(_waterShader, "assets/opengl/vs_water.glsl", "assets/opengl/fs_water.glsl");
    createShader(_directionalLightShader, "assets/opengl/vs_dl.glsl", "assets/opengl/fs_dl.glsl");
    createShader(_waterPopUpDistortionShader, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_water_popup_distortion.glsl");
    createShader(_underwaterShader, "assets/opengl/vs_screen.glsl", "assets/opengl/fs_underwater.glsl");
  }

  void RenderingSystemOpenGL::onDirectionalLightAdded(Entity* entity, bool always_create)
  {
    _lightQueries[entity] = std::make_shared<GLQuery>(GL_SAMPLES_PASSED);
    if (_shadowMaps.count(entity) && !always_create) {
      return;
    }

    auto dl = entity->getComponent<DirectionalLight>();
    auto num_cascades = _settings._smFrustumSplits.size();

    glm::ivec2 size(1024);

    auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D_ARRAY);
    texture->create();
    texture->bind();
    texture->setMagnificationFilter(GL_LINEAR);
    texture->setMinificationFilter(GL_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    texture->setCompareMode(GL_COMPARE_REF_TO_TEXTURE, GL_LESS);
    texture->setData(size.x, size.y, num_cascades, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    _shadowMaps[entity] = std::make_shared<GLFramebufferOld>();
    _shadowMaps[entity]->create(size.x, size.y);
    _shadowMaps[entity]->bind();
    _shadowMaps[entity]->addTexture(texture, GL_DEPTH_ATTACHMENT);
    _shadowMaps[entity]->setDrawbuffers({ GL_NONE });
    _shadowMaps[entity]->checkStatus();
  }

  void RenderingSystemOpenGL::onLightRemoved(Entity* entity)
  {
    _shadowMaps.erase(entity);
    _lightQueries.erase(entity);
  }

  void RenderingSystemOpenGL::onTerrainAdded(Entity * entity)
  {
    _terrainRenderables[entity] = (std::make_shared<TerrainRenderable>(entity, this));
  }

  void RenderingSystemOpenGL::onTerrainRemoved(Entity * entity)
  {
    _terrainRenderables.erase(entity);
  }

  void RenderingSystemOpenGL::onResize(const glm::ivec2 & size)
  {
    RenderingSystem::onResize(size);
    initFramebuffers();
    _aspectRatio = static_cast<float>(size.x) / size.y;
  }

  void RenderingSystemOpenGL::onSpotLightAdded(Entity * entity, bool always_create)
  {
    _lightQueries[entity] = std::make_shared<GLQuery>(GL_SAMPLES_PASSED);

    if (_shadowMaps.count(entity) && !always_create) {
      return;
    }

    glm::ivec2 size(1024);
    auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    texture->create();
    texture->bind();
    texture->setMagnificationFilter(GL_LINEAR);
    texture->setMinificationFilter(GL_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    texture->setCompareMode(GL_COMPARE_REF_TO_TEXTURE, GL_LEQUAL);
    texture->setData(size.x, size.y, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    auto fb = std::make_shared<GLFramebufferOld>();
    fb->create(size.x, size.y);
    fb->bind();
    fb->addTexture(texture, GL_DEPTH_ATTACHMENT);
    fb->setDrawbuffers({ GL_NONE });
    fb->checkStatus();
    _shadowMaps[entity] = fb;
  }

  void RenderingSystemOpenGL::onPointLightAdded(Entity * entity, bool always_create)
  {
    _lightQueries[entity] = std::make_shared<GLQuery>(GL_SAMPLES_PASSED);
    if (_shadowMaps.count(entity) && !always_create) {
      return;
    }
    glm::ivec2 size(1024);
    auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_CUBE_MAP);
    texture->create();
    texture->bind();
    texture->setMagnificationFilter(GL_LINEAR);
    texture->setMinificationFilter(GL_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
    texture->setCompareMode(GL_COMPARE_REF_TO_TEXTURE, GL_LEQUAL);
    for (int i = 0; i < 6; i++) {
      GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT16, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));
    }
    auto fb = std::make_shared<GLFramebufferOld>();
    fb->create(size.x, size.y);
    fb->bind();
    fb->addTexture(texture, GL_DEPTH_ATTACHMENT);
    fb->setDrawbuffers({ GL_NONE });
    fb->checkStatus();
    _shadowMaps[entity] = fb;
  }

  void RenderingSystemOpenGL::renderMeshGeometryDeferred(Entity* entity)
  {
  }

  bool RenderingSystemOpenGL::isCulled(const std::array<glm::vec3, 8>& bb_modelspace, const glm::mat4& mvp)
  {
    std::array<glm::vec4, 8> bb_screen;
    for (unsigned int i = 0; i < 8; i++) {
      bb_screen[i] = mvp * glm::vec4(bb_modelspace[i], 1.f);
    }
    bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_near = false, inside_far = false;
    for (auto& p : bb_screen) {
      inside_left = inside_left || p.x >= -p.w;
      inside_right = inside_right || p.x <= p.w;
      inside_bottom = inside_bottom || p.y >= -p.w;
      inside_top = inside_top || p.y <= p.w;
      inside_near = inside_near || p.z >= -p.w;
      inside_far = inside_far || p.z <= p.w;
    }
    bool inside = inside_left && inside_right && inside_bottom && inside_top && inside_near && inside_far;
    return !inside;
  }

  bool RenderingSystemOpenGL::isCulled(const std::shared_ptr<Mesh>& mesh, const glm::mat4 & model_matrix)
  {
    auto mvp = _VP * model_matrix;
    return !mesh->getAABB()->isVisible<false, false>(mvp);
  }

  void RenderingSystemOpenGL::initFramebuffers()
  {
    for (auto& dl : _directionalLights) {
      onDirectionalLightAdded(dl, true);
    }
    for (auto& sl : _spotLights) {
      onSpotLightAdded(sl, true);
    }
    for (auto& pl : _pointLights) {
      onPointLightAdded(pl, true);
    }

    _bloomStages.clear();
    _bloomStages.push_back(BloomStage(_viewportSize, 0));
    _bloomStages.push_back(BloomStage(_viewportSize, 1));
    _bloomStages.push_back(BloomStage(_viewportSize, 2));

    _godRayEffect = std::make_shared<GodRayEffect>(_viewportSize);

    _gBuffer = std::make_shared<GLFramebufferOld>();
    _gBuffer->create(_viewportSize.x, _viewportSize.y);
    _gBuffer->bind();

    auto tex_array = std::make_shared<GLTextureOld>(GL_TEXTURE_2D_ARRAY);
    tex_array->create();
    tex_array->bind();
    tex_array->setMagnificationFilter(GL_NEAREST);
    tex_array->setMinificationFilter(GL_NEAREST);
    unsigned int num_attachments = 6;
    tex_array->setData(_viewportSize.x, _viewportSize.y, num_attachments, GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
    std::vector<GLenum> attachments;
    for (unsigned int i = 0; i < num_attachments; i++) {
      attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
      _gBuffer->addTextureLayer(tex_array, attachments.back(), i);
    }
    _gBuffer->setDrawbuffers(attachments);

    _sceneDepthbuffer = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    _sceneDepthbuffer->create();
    _sceneDepthbuffer->bind();
    _sceneDepthbuffer->setMinificationFilter(GL_NEAREST);
    _sceneDepthbuffer->setMagnificationFilter(GL_NEAREST);
    _sceneDepthbuffer->setData(_viewportSize.x, _viewportSize.y, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    _gBuffer->setDepthTexture(_sceneDepthbuffer);
    _gBuffer->checkStatus();

    for (unsigned int i = 0; i < 2; i++) {
      _framebufferQuarterSize[i] = std::make_shared<GLFramebufferOld>();
      auto size = _viewportSize / 4;
      _framebufferQuarterSize[i]->create(size.x, size.y);
      _framebufferQuarterSize[i]->bind();
      auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setMinificationFilter(GL_LINEAR);
      texture->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      texture->setData(_framebufferQuarterSize[i]->width(), _framebufferQuarterSize[i]->height(), GL_R16F, GL_RED, GL_FLOAT, nullptr);
      _framebufferQuarterSize[i]->addTexture(texture, GL_COLOR_ATTACHMENT0);
      _framebufferQuarterSize[i]->setDrawbuffersFromColorAttachments();
    }

    for (unsigned int i = 0; i < 2; i++) {
      _gradientBufferXQuarterSize[i] = std::make_shared<GLFramebufferOld>();
      auto size = _viewportSize / 4;
      _gradientBufferXQuarterSize[i]->create(size.x, size.y);
      _gradientBufferXQuarterSize[i]->bind();
      auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setMinificationFilter(GL_LINEAR);
      texture->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      texture->setData(_gradientBufferXQuarterSize[i]->width(), _gradientBufferXQuarterSize[i]->height(), GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
      _gradientBufferXQuarterSize[i]->addTexture(texture, GL_COLOR_ATTACHMENT0);
      _gradientBufferXQuarterSize[i]->setDrawbuffersFromColorAttachments();
    }

    for (unsigned int i = 0; i < 2; i++) {
      _gradientBufferYQuarterSize[i] = std::make_shared<GLFramebufferOld>();
      auto size = _viewportSize / 4;
      _gradientBufferYQuarterSize[i]->create(size.x, size.y);
      _gradientBufferYQuarterSize[i]->bind();
      auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setMinificationFilter(GL_LINEAR);
      texture->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      texture->setData(_gradientBufferYQuarterSize[i]->width(), _gradientBufferYQuarterSize[i]->height(), GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
      _gradientBufferYQuarterSize[i]->addTexture(texture, GL_COLOR_ATTACHMENT0);
      _gradientBufferYQuarterSize[i]->setDrawbuffersFromColorAttachments();
    }

    _gradientBufferQuarterSize = std::make_shared<GLFramebufferOld>();
    auto size = _viewportSize / 4;
    _gradientBufferQuarterSize->create(size.x, size.y);
    _gradientBufferQuarterSize->bind();
    auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
    texture->create();
    texture->bind();
    texture->setMinificationFilter(GL_LINEAR);
    texture->setMagnificationFilter(GL_LINEAR);
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    std::vector<GLint> swizzle_mask = { GL_RED, GL_RED, GL_RED, GL_RED };
    GL_CHECK(glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, &swizzle_mask[0]));
    texture->setData(_gradientBufferQuarterSize->width(), _gradientBufferQuarterSize->height(), GL_R16F, GL_RED, GL_FLOAT, nullptr);
    _gradientBufferQuarterSize->addTexture(texture, GL_COLOR_ATTACHMENT0);
    _gradientBufferQuarterSize->setDrawbuffersFromColorAttachments();

    int i = 0;
    for (auto& lb : _lightingBuffer) {
      lb = std::make_shared<GLFramebufferOld>();
      lb->create(_viewportSize.x, _viewportSize.y);
      lb->bind();
      texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setMinificationFilter(GL_NEAREST);
      texture->setMagnificationFilter(GL_NEAREST);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      texture->setData(_viewportSize.x, _viewportSize.y, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
      lb->addTexture(texture, GL_COLOR_ATTACHMENT0);
      if (i == 0) {
        lb->setDepthTexture(_sceneDepthbuffer);
      }
      lb->setDrawbuffersFromColorAttachments();
      lb->checkStatus();
      i++;
    }

    for (unsigned int i = 0; i < 2; i++) {
      auto fb = std::make_shared<GLFramebufferOld>();
      fb->create(1, 1);
      fb->bind();
      texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setData(1, 1, GL_R16F, GL_RED, GL_FLOAT, &_exposure);
      fb->addTexture(texture, GL_COLOR_ATTACHMENT0);
      fb->setDrawbuffersFromColorAttachments();
      fb->checkStatus();
      _exposureBuffer[i] = fb;
    }

    for (unsigned int i = 0; i < 2; i++) {
      _DOFBlurBuffer[i] = std::make_shared<GLFramebufferOld>();
      _DOFBlurBuffer[i]->create(_viewportSize.x / 2, _viewportSize.y / 2);
      _DOFBlurBuffer[i]->bind();
      auto texture = std::make_shared<GLTextureOld>(GL_TEXTURE_2D);
      texture->create();
      texture->bind();
      texture->setMinificationFilter(GL_LINEAR);
      texture->setMagnificationFilter(GL_LINEAR);
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
      texture->setData(_DOFBlurBuffer[i]->width(), _DOFBlurBuffer[i]->height(), GL_RGB16F, GL_RGB, GL_FLOAT, nullptr);
      _DOFBlurBuffer[i]->addTexture(texture, GL_COLOR_ATTACHMENT0);
      _DOFBlurBuffer[i]->setDrawbuffersFromColorAttachments();
    }
  }
  float RenderingSystemOpenGL::getSceneDepth(const glm::ivec2 & pos)
  {
    _gBuffer->bind();
    float depth;
    GL_CHECK(glReadPixels(pos.x, pos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth));
    return depth;
    /* _sceneDepthbuffer->bind();
     std::vector<float> depth(_readBuffer->width() * _readBuffer->height());
     glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depth[0]);
     return depth [pos.y * _readBuffer->width() + pos.x];*/
  }
  void RenderingSystemOpenGL::setDefaultFramebufferId(unsigned int fb_id)
  {
    _defaultFramebufferId = fb_id;
  }

  void fly::RenderingSystemOpenGL::getSobelKernel(std::vector<float>& smooth, std::vector<float>& gradient)
  {
    smooth = _sobelKernelSmooth;
    gradient = _sobelKernelGradient;
  }
  void fly::RenderingSystemOpenGL::setSettings(const Settings & settings)
  {
    _settings = settings;
  }
  const Settings & fly::RenderingSystemOpenGL::getSettings() const
  {
    return _settings;
  }
}