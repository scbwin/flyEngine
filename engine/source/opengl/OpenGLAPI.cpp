#include <opengl/OpenGLAPI.h>
#include <iostream>
#include <opengl/GLVertexArray.h>
#include <opengl/GLBuffer.h>
#include <Model.h>
#include <Vertex.h>
#include <Mesh.h>
#include <opengl/GLWrappers.h>
#include <Timing.h>
#include <StaticModelRenderable.h>
#include <SOIL/SOIL.h>
#include <opengl/GLAppendBuffer.h>
#include <Material.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLFramebuffer.h>
#include <opengl/GLSLShaderGenerator.h>
#include <Settings.h>
#include <renderer/RenderParams.h>

namespace fly
{
  OpenGLAPI::OpenGLAPI() : 
    _shaderGenerator(std::make_unique<GLSLShaderGenerator>())
  {
    glewExperimental = true;
    auto result = glewInit();
    if (result == GLEW_OK) {
      GLint major_version, minor_version;
      GL_CHECK(glGetIntegerv(GL_MAJOR_VERSION, &major_version));
      GL_CHECK(glGetIntegerv(GL_MINOR_VERSION, &minor_version));
      std::cout << "OpenGLAPI::OpenGLAPI(): Initialized GLEW, GL Version: " << major_version << "." << minor_version << std::endl;
    }
    else {
      std::cout << "OpenGLAPI::OpenGLAPI() Failed to initialized GLEW: " << glewGetErrorString(result) << std::endl;
    }
    _aabbShader = createShader("assets/opengl/vs_aabb.glsl", "assets/opengl/fs_aabb.glsl", "assets/opengl/gs_aabb.glsl");
    _compositeShader = createShader("assets/opengl/vs_screen.glsl", "assets/opengl/fs_composite.glsl");
    _vaoAABB = std::make_shared<GLVertexArray>();
    _vaoAABB->bind();
    _vboAABB = std::make_shared<GLBuffer>(GL_ARRAY_BUFFER);
    _vboAABB->bind();
    for (unsigned i = 0; i < 2; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
      GL_CHECK(glVertexAttribDivisor(i, 1));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), 0));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(Vec3f), reinterpret_cast<const void*>(sizeof(Vec3f))));

    _offScreenFramebuffer = std::make_unique<GLFramebuffer>();
  }
  OpenGLAPI::~OpenGLAPI()
  {
  }
  ZNearMapping OpenGLAPI::getZNearMapping() const
  {
    return ZNearMapping::MINUS_ONE;
  }
  void OpenGLAPI::setViewport(const Vec2u & size) const
  {
    GL_CHECK(glViewport(0, 0, size[0], size[1]));
  }
  void OpenGLAPI::reloadShaders()
  {
    for (const auto& e : _shaderCache) {
      e.second->reload();
    }
  }
  void OpenGLAPI::setupShader(GLShaderProgram * shader)
  {
    _activeShader = shader;
    _activeShader->bind();
  }
  void OpenGLAPI::setupShader(GLShaderProgram* shader, const GlobalShaderParams& param)
  {
    setupShader(shader);
    setupShaderConstants(param);
  }
  void OpenGLAPI::setupShader(GLShaderProgram * shader, const GlobalShaderParams& param, const Depthbuffer* shadow_map)
  {
    setupShader(shader);
    setupShaderConstants(param, shadow_map);
  }
  void OpenGLAPI::setupShaderConstants(const GlobalShaderParams& param)
  {
    setMatrix(_activeShader->uniformLocation("P"), param._projectionMatrix);
    setVector(_activeShader->uniformLocation("lpos_cs"), param._lightPosView);
    setVector(_activeShader->uniformLocation("I_in"), param._lightIntensity);
  }
  void OpenGLAPI::setupShaderConstants(const GlobalShaderParams& param, const Depthbuffer* shadow_map)
  {
    setupShaderConstants(param);
    GL_CHECK(glActiveTexture(GL_TEXTURE3));
    shadow_map->bind();
    setScalar(_activeShader->uniformLocation("ts_sm"), 3);
    setMatrixArray(_activeShader->uniformLocation("v_to_l"), param._viewToLight.front(), static_cast<unsigned>(param._viewToLight.size()));
    setScalar(_activeShader->uniformLocation("nfs"), static_cast<int>(param._viewToLight.size()));
    setScalarArray(_activeShader->uniformLocation("fs"), param._smFrustumSplits.front(), static_cast<unsigned>(param._smFrustumSplits.size()));
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc, const GlobalShaderParams& param, const Depthbuffer* shadow_map)
  {
    setupShader(desc.getShader().get());
    setupShaderConstants(param, shadow_map);
    desc.setup();
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc, const GlobalShaderParams& param)
  {
    setupShader(desc.getShader().get());
    setupShaderConstants(param);
    desc.setup();
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mv)
  {
    setMatrix(_activeShader->uniformLocation("MV"), mv);
    setMatrixTranspose(_activeShader->uniformLocation("MV_i"), inverse(glm::mat3(mv)));
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderMeshMVP(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mvp)
  {
    setMatrix(_activeShader->uniformLocation("MVP"), mvp);
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
  }
  void OpenGLAPI::renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col)
  {
    _vaoAABB->bind();
    setupShader(_aabbShader.get());
    setMatrix(_activeShader->uniformLocation("VP"), transform);
    setVector(_activeShader->uniformLocation("c"), col);
    std::vector<Vec3f> bb_buffer;
    for (const auto& aabb : aabbs) {
      bb_buffer.push_back(aabb->getMin());
      bb_buffer.push_back(aabb->getMax());
    }
    _vboAABB->setData(bb_buffer.data(), bb_buffer.size(), GL_DYNAMIC_COPY);
    GL_CHECK(glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(aabbs.size())));
  }
  void OpenGLAPI::setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer->texture(GL_DEPTH_ATTACHMENT, depth_buffer, 0);
    checkFramebufferStatus();
  }
  void OpenGLAPI::setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer)
  {
    setColorBuffers(rtts);
    _offScreenFramebuffer->textureLayer(GL_DEPTH_ATTACHMENT, depth_buffer, 0, depth_buffer_layer);
    checkFramebufferStatus();
  }
  void OpenGLAPI::bindBackbuffer(unsigned id) const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, id));
  }
  void OpenGLAPI::composite(const RTT* lighting_buffer)
  {
    _compositeShader->bind();
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    lighting_buffer->bind();
    setScalar(_compositeShader->uniformLocation("ts_l"), 0);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  std::shared_ptr<GLTexture> OpenGLAPI::createTexture(const std::string & path)
  {
    if (path == "") {
      return nullptr;
    }
    auto it = _textureCache.find(path);
    if (it != _textureCache.end()) {
      return it->second;
    }
    auto tex = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
    if (tex == 0) {
      return nullptr;
    }
    auto ret = std::make_shared<GLTexture>(tex, GL_TEXTURE_2D);
    _textureCache[path] = ret;
    return ret;
  }
  std::shared_ptr<OpenGLAPI::MaterialDesc> OpenGLAPI::createMaterial(const std::shared_ptr<Material>& material, const Settings& settings)
  {
  //  std::cout << "mat desc cache size:" << _matDescCache.size() << std::endl;
    auto it = _matDescCache.find(material);
    if (it != _matDescCache.end()) {
      return it->second;
    }
    auto ret = std::make_shared<MaterialDesc>(material, this, settings);
    _matDescCache[material] = ret;
    return ret;
  }
  std::shared_ptr<GLShaderProgram> OpenGLAPI::createShader(const std::string & vertex_file, const std::string & fragment_file, const std::string& geometry_file)
  {
    std::string key = vertex_file + fragment_file + geometry_file;
    auto it = _shaderCache.find(key);
    if (it != _shaderCache.end()) {
      return it->second;
    }
    auto ret = std::make_shared<GLShaderProgram>();
    ret->create();
    ret->addShaderFromFile(vertex_file, GLShaderProgram::ShaderType::VERTEX);
    if (geometry_file != "") {
      ret->addShaderFromFile(geometry_file, GLShaderProgram::ShaderType::GEOMETRY);
    }
    ret->addShaderFromFile(fragment_file, GLShaderProgram::ShaderType::FRAGMENT);
    ret->link();
    _shaderCache[key] = ret;
    return ret;
  }
  std::unique_ptr<OpenGLAPI::RTT> OpenGLAPI::createRenderToTexture(const Vec2u & size)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_RGBA16F, size, 0, GL_RGBA, GL_FLOAT, nullptr);
    return tex;
  }
  std::unique_ptr<OpenGLAPI::Depthbuffer> OpenGLAPI::createDepthbuffer(const Vec2u & size)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_DEPTH_COMPONENT24, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
  }
  std::unique_ptr<OpenGLAPI::Shadowmap> OpenGLAPI::createShadowmap(const Vec2u & size, const Settings& settings)
  {
    auto tex = std::make_unique<GLTexture>(GL_TEXTURE_2D_ARRAY);
    tex->bind();
    GLint filter = settings._shadowPercentageCloserFiltering ? GL_LINEAR : GL_NEAREST;
    tex->param(GL_TEXTURE_MIN_FILTER, filter);
    tex->param(GL_TEXTURE_MAG_FILTER, filter);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    tex->param(GL_TEXTURE_BORDER_COLOR, Vec4f(std::numeric_limits<float>::max()).ptr());
    if (settings._shadowPercentageCloserFiltering) {
      tex->param(GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      tex->param(GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
    }
    tex->image3D(0, GL_DEPTH_COMPONENT24, Vec3u(size, static_cast<unsigned>(settings._smFrustumSplits.size())), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
  }
  void OpenGLAPI::recreateShadersAndMaterials(const Settings& settings)
  {
    _shaderGenerator->regenerateShaders(settings);
    _shaderCache.clear();
    for (const auto e : _matDescCache) {
      e.second->create(this, settings);
    }
  }
  void OpenGLAPI::checkFramebufferStatus()
  {
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Framebuffer imcomplete" << std::endl;
    }
  }
  void OpenGLAPI::setColorBuffers(const std::vector<RTT*>& rtts)
  {
    _offScreenFramebuffer->bind();
    _offScreenFramebuffer->clearAttachments();
    std::vector<GLenum> draw_buffers;
    if (rtts.size()) {
      unsigned i = 0;
      for (const auto& rtt : rtts) {
        draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        _offScreenFramebuffer->texture(draw_buffers.back(), rtt, 0);
        i++;
      }
    }
    else {
      draw_buffers.push_back(GL_NONE);
    }
    GL_CHECK(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
  }
  OpenGLAPI::MeshGeometryStorage::MeshGeometryStorage() :
    _vboAppend(std::make_unique<GLAppendBuffer>(GL_ARRAY_BUFFER)),
    _iboAppend(std::make_unique<GLAppendBuffer>(GL_ELEMENT_ARRAY_BUFFER))
  {
  }
  OpenGLAPI::MeshGeometryStorage::~MeshGeometryStorage()
  {
  }
  void OpenGLAPI::MeshGeometryStorage::bind() const
  {
    _vao->bind();
  }
  OpenGLAPI::MeshGeometryStorage::MeshData OpenGLAPI::MeshGeometryStorage::addMesh(const std::shared_ptr<Mesh>& mesh)
  {
    auto it = _meshDataCache.find(mesh);
    if (it != _meshDataCache.end()) { // Mesh is already in the cache
      return it->second;
    }
    MeshData data;
    data._count = static_cast<GLsizei>(mesh->getIndices().size());
    data._baseVertex = static_cast<GLint>(_baseVertex);
    data._indices = reinterpret_cast<GLvoid*>(_indices);
    _indices += mesh->getIndices().size() * sizeof(mesh->getIndices().front());
    _baseVertex += mesh->getVertices().size();
    _vboAppend->appendData(mesh->getVertices().data(), mesh->getVertices().size());
    _iboAppend->appendData(mesh->getIndices().data(), mesh->getIndices().size());
    _vao = std::make_unique<GLVertexArray>();
    _vao->bind();
    _vboAppend->getBuffer()->bind();
    _iboAppend->getBuffer()->bind();
    for (unsigned i = 0; i < 5; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _position))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _normal))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _uv))));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _tangent))));
    GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _bitangent))));
    GL_CHECK(glBindVertexArray(0));
    _meshDataCache[mesh] = data;
    return data;
  }
  OpenGLAPI::MaterialDesc::MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI * api, const Settings& settings) : _material(material)
  {
    create(material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(OpenGLAPI * api, const Settings & settings)
  {
    create(_material, api, settings);
  }
  void OpenGLAPI::MaterialDesc::create(const std::shared_ptr<Material>& material, OpenGLAPI* api, const Settings& settings)
  {
    _shaderSetupFuncs.clear();
    _diffuseMap = api->createTexture(material->getDiffusePath());
    _normalMap = api->createTexture(material->getNormalPath());
    _alphaMap = api->createTexture(material->getOpacityPath());
    using FLAG = GLSLShaderGenerator::MeshRenderFlag;
    unsigned flag = FLAG::NONE;
    std::string vertex_file = "assets/opengl/vs_simple.glsl";
    if (_diffuseMap) {
      flag |= FLAG::DIFFUSE_MAP;
      _shaderSetupFuncs.push_back([this]() {
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        _diffuseMap->bind();
        setScalar(_shader->uniformLocation("ts_diff"), 0);
      });
    }
    else {
      _shaderSetupFuncs.push_back([this]() {
        setVector(_shader->uniformLocation("d_col"), _material->getDiffuseColor());
      });
    }
    if (_alphaMap) {
      flag |= FLAG::ALPHA_MAP;
      _shaderSetupFuncs.push_back([this]() {
        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        _alphaMap->bind();
        setScalar(_shader->uniformLocation("ts_alpha"), 1);
      });
    }
    if (_normalMap && settings._normalMapping) {
      flag |= FLAG::NORMAL_MAP;
      _shaderSetupFuncs.push_back([this]() {
        GL_CHECK(glActiveTexture(GL_TEXTURE2));
        _normalMap->bind();
        setScalar(_shader->uniformLocation("ts_norm"), 2);
      });
    }
    _shader = api->createShader(vertex_file, api->_shaderGenerator->createMeshFragmentShaderFile(flag, settings));
    _smShader = api->createShader("assets/opengl/vs_shadow.glsl", "assets/opengl/fs_shadow.glsl");
    assert(_shaderSetupFuncs.size() <= 3);
  }
  void OpenGLAPI::MaterialDesc::setup() const
  {
    for (const auto& f : _shaderSetupFuncs) {
      f();
    }
    setScalar(_shader->uniformLocation("ka"), _material->getKa());
    setScalar(_shader->uniformLocation("kd"), _material->getKd());
    setScalar(_shader->uniformLocation("ks"), _material->getKs());
    setScalar(_shader->uniformLocation("s_e"), _material->getSpecularExponent());
  }
  const std::shared_ptr<OpenGLAPI::MaterialDesc::ShaderProgram>& OpenGLAPI::MaterialDesc::getShader() const
  {
    return _shader;
  }
  const std::shared_ptr<OpenGLAPI::MaterialDesc::ShaderProgram>& OpenGLAPI::MaterialDesc::getSMShader() const
  {
    return _smShader;
  }
}