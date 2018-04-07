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
#include <opengl/GLTexture.h>
#include <opengl/GLAppendBuffer.h>
#include <Material.h>
#include <opengl/GLMaterialSetup.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLFramebuffer.h>

namespace fly
{
  OpenGLAPI::OpenGLAPI()
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
  void OpenGLAPI::setupShader(GLShaderProgram* shader, const Vec3f & dl_pos_view_space, const Mat4f & projection_matrix, const Vec3f& light_intensity)
  {
    _activeShader = shader;
    _activeShader->bind();
    setupShaderConstants(dl_pos_view_space, projection_matrix, light_intensity);
  }
  void OpenGLAPI::setupShader(GLShaderProgram * shader, const Vec3f & dl_pos_view_space, const Mat4f & projection_matrix, 
    const Vec3f & light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light)
  {
    _activeShader = shader;
    _activeShader->bind();
    setupShaderConstants(dl_pos_view_space, projection_matrix, light_intensity, shadow_map, view_to_light);
  }
  void OpenGLAPI::setupShaderConstants(const Vec3f & dl_pos_view_space, const Mat4f & projection_matrix, const Vec3f& light_intensity)
  {
    setMatrix(_activeShader->uniformLocation("P"), projection_matrix);
    setVector(_activeShader->uniformLocation("lpos_cs"), dl_pos_view_space);
    setVector(_activeShader->uniformLocation("I_in"), light_intensity);
  }
  void OpenGLAPI::setupShaderConstants(const Vec3f & dl_pos_view_space, const Mat4f & projection_matrix, const Vec3f & light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light)
  {
    setupShaderConstants(dl_pos_view_space, projection_matrix, light_intensity);
    GL_CHECK(glActiveTexture(GL_TEXTURE3));
    shadow_map->bind();
    setScalar(_activeShader->uniformLocation("ts_sm"), 3);
    setMatrix(_activeShader->uniformLocation("v_to_l"), view_to_light);
  }
  void OpenGLAPI::setupMaterialConstants(const std::shared_ptr<Material>& material)
  {
    setScalar(_activeShader->uniformLocation("ka"), material->getKa());
    setScalar(_activeShader->uniformLocation("kd"), material->getKd());
    setScalar(_activeShader->uniformLocation("ks"), material->getKs());
    setScalar(_activeShader->uniformLocation("s_e"), material->getSpecularExponent());
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc)
  {
    desc.getMaterialSetup()->setup(desc);
    setupMaterialConstants(desc.getMaterial());
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc, const Vec3f & dl_pos_view_space, const Mat4f & projection_matrix,
    const Vec3f & light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light)
  {
    _activeShader = desc.getShader().get();
    _activeShader->bind();
    setupShaderConstants(dl_pos_view_space, projection_matrix, light_intensity, shadow_map, view_to_light);
    setupMaterialConstants(desc.getMaterial());
    desc.getMaterialSetup()->setup(desc);
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity)
  {
    _activeShader = desc.getShader().get();
    _activeShader->bind();
    setupShaderConstants(dl_pos_view_space, projection_matrix, light_intensity);
    setupMaterialConstants(desc.getMaterial());
    desc.getMaterialSetup()->setup(desc);
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mv)
  {
    setMatrix(_activeShader->uniformLocation("MV"), mv);
    setMatrixTranspose(_activeShader->uniformLocation("MV_i"), inverse(mv));
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
    _activeShader = _aabbShader.get();
    _activeShader->bind();
    setMatrix(_activeShader->uniformLocation("VP"), transform);
    std::vector<Vec3f> bb_buffer;
    for (const auto& aabb : aabbs) {
      bb_buffer.push_back(aabb->getMin());
      bb_buffer.push_back(aabb->getMax());
    }
    setVector(_activeShader->uniformLocation("c"), col);
    _vboAABB->setData(bb_buffer.data(), bb_buffer.size(), GL_DYNAMIC_COPY);
    GL_CHECK(glDrawArraysInstanced(GL_POINTS, 0, 1, aabbs.size()));
  }
  void OpenGLAPI::setRendertargets(const std::vector<std::shared_ptr<RTT>>& rtts, const std::shared_ptr<Depthbuffer>& depth_buffer)
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
    _offScreenFramebuffer->texture(GL_DEPTH_ATTACHMENT, depth_buffer, 0);
    GL_CHECK(glDrawBuffers(draw_buffers.size(), draw_buffers.data()));
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
      std::cout << "Framebuffer imcomplete" << std::endl;
    }
  }
  void OpenGLAPI::bindBackbuffer(unsigned id) const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, id));
  }
  void OpenGLAPI::composite(const std::shared_ptr<RTT>& lighting_buffer)
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
  std::shared_ptr<OpenGLAPI::MaterialDesc> OpenGLAPI::createMaterial(const std::shared_ptr<Material>& material)
  {
    auto it = _matDescCache.find(material);
    if (it != _matDescCache.end()) {
      return it->second;
    }
    auto ret = std::make_shared<MaterialDesc>(material, this);
    _matDescCache[material] = ret;
    return ret;
  }
  std::shared_ptr<GLShaderProgram> OpenGLAPI::createShader(const std::string & vertex_file, const std::string & fragment_file, const std::string& geometry_file)
  {
    std::string key = vertex_file + fragment_file;
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
  std::shared_ptr<OpenGLAPI::RTT> OpenGLAPI::createRenderToTexture(const Vec2u & size)
  {
    auto tex = std::make_shared<GLTexture>();
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_RGBA16F, size, 0, GL_RGBA, GL_FLOAT, nullptr);
    return tex;
  }
  std::shared_ptr<OpenGLAPI::Depthbuffer> OpenGLAPI::createDepthbuffer(const Vec2u & size)
  {
    auto tex = std::make_shared<GLTexture>();
    tex->bind();
    tex->param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    tex->param(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    tex->param(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    tex->image2D(0, GL_DEPTH_COMPONENT24, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    return tex;
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
  OpenGLAPI::MaterialDesc::MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI * api) : _material(material)
  {
    _diffuseMap = api->createTexture(material->getDiffusePath());
    _normalMap = api->createTexture(material->getNormalPath());
    _alphaMap = api->createTexture(material->getOpacityPath());
    std::string vertex_file = "assets/opengl/vs_simple.glsl";
    std::string fragment_file = "assets/opengl/fs_simple";
    if (_diffuseMap && _alphaMap && _normalMap) {
      _materialSetup = std::make_unique<SetupDiffuseAlphaNormalMap>();
      fragment_file += "_textured_alpha_normal";
    }
    else if (_diffuseMap && _normalMap) {
      _materialSetup = std::make_unique<SetupDiffuseNormalMap>();
      fragment_file += "_textured_normal";
    }
    else if (_diffuseMap && _alphaMap) {
      _materialSetup = std::make_unique<SetupDiffuseAlphaMap>();
      fragment_file += "_textured_alpha";
    }
    else if (_diffuseMap) {
      _materialSetup = std::make_unique<SetupDiffuseMap>();
      fragment_file += "_textured";
    }
    else if (_alphaMap) {
      _materialSetup = std::make_unique<SetupAlphaMap>();
      std::string err = "Unsupported material type.";
      throw std::exception(err.c_str());
    }
    else {
      _materialSetup = std::make_unique<SetupDiffuseColor>();
      fragment_file += "_color";
    }
    fragment_file += ".glsl";
    _shader = api->createShader(vertex_file, fragment_file);
    _smShader = api->createShader("assets/opengl/vs_shadow.glsl", "assets/opengl/fs_shadow.glsl");
  }
  const std::unique_ptr<GLMaterialSetup>& OpenGLAPI::MaterialDesc::getMaterialSetup() const
  {
    return _materialSetup;
  }
  const std::shared_ptr<GLShaderProgram>& OpenGLAPI::MaterialDesc::getShader() const
  {
    return _shader;
  }
  const std::shared_ptr<GLShaderProgram>& OpenGLAPI::MaterialDesc::getSMShader() const
  {
    return _smShader;
  }
  const std::shared_ptr<Material>& OpenGLAPI::MaterialDesc::getMaterial() const
  {
    return _material;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::getDiffuseMap() const
  {
    return _diffuseMap;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::getNormalMap() const
  {
    return _normalMap;
  }
  const std::shared_ptr<GLTexture>& OpenGLAPI::MaterialDesc::getAlphaMap() const
  {
    return _alphaMap;
  }
}