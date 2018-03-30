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
    Timing timing;
    initShaders();
    std::cout << "OpenGLAPI::initShaders() took " << timing << std::endl;
  }
  ZNearMapping OpenGLAPI::getZNearMapping() const
  {
    return ZNearMapping::MINUS_ONE;
  }
  void OpenGLAPI::initShaders()
  {
    _simpleFullscreenQuadShader = std::make_shared<GLShaderProgram>();
    _simpleFullscreenQuadShader->create();
    _simpleFullscreenQuadShader->addShaderFromFile("assets/opengl/vs_screen.glsl", GLShaderProgram::ShaderType::VERTEX);
    _simpleFullscreenQuadShader->addShaderFromFile("assets/opengl/fs_simple.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _simpleFullscreenQuadShader->link();

    _diffuseShader = std::make_shared<GLShaderProgram>();
    _diffuseShader->create();
    _diffuseShader->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _diffuseShader->addShaderFromFile("assets/opengl/fs_simple_textured.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _diffuseShader->link();

    _alphaShader = std::make_shared<GLShaderProgram>();
    _alphaShader->create();
    _alphaShader->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _alphaShader->addShaderFromFile("assets/opengl/fs_simple_textured_alpha.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _alphaShader->link();

    _colorShader = std::make_shared<GLShaderProgram>();
    _colorShader->create();
    _colorShader->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _colorShader->addShaderFromFile("assets/opengl/fs_simple_color.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _colorShader->link();
  }
  void OpenGLAPI::setViewport(const Vec2u & size) const
  {
    GL_CHECK(glViewport(0, 0, size[0], size[1]));
  }
  void OpenGLAPI::renderFullScreenQuad() const
  {
    _simpleFullscreenQuadShader->bind();
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
  void OpenGLAPI::clearRendertargetColor(const Vec4f & color) const
  {
    GL_CHECK(glClearColor(color[0], color[1], color[2], color[3]));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
  }
  void OpenGLAPI::setupMaterial(const MaterialDesc & desc, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix)
  {
    _activeShader = desc.getShader();
    _activeShader->bind();
    GL_CHECK(glUniformMatrix4fv(_activeShader->uniformLocation("P"), 1, false, projection_matrix.ptr()));
    GL_CHECK(glUniform3f(_activeShader->uniformLocation("lpos_cs"), dl_pos_view_space[0], dl_pos_view_space[1], dl_pos_view_space[2]));
    desc.getMaterialSetup()->setup(desc);
  }
  void OpenGLAPI::renderMesh(const MeshGeometryStorage::MeshData & mesh_data, const Mat4f & mv)
  {
    GL_CHECK(glUniformMatrix4fv(_activeShader->uniformLocation("MV"), 1, false, mv.ptr()));
    GL_CHECK(glUniformMatrix4fv(_activeShader->uniformLocation("MV_i"), 1, true, inverse(mv).ptr()));
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_data._count, GL_UNSIGNED_INT, mesh_data._indices, mesh_data._baseVertex));
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
  OpenGLAPI::MeshGeometryStorage::MeshGeometryStorage() : 
    _vboAppend(std::make_shared<GLAppendBuffer>(GL_ARRAY_BUFFER)),
    _iboAppend(std::make_shared<GLAppendBuffer>(GL_ELEMENT_ARRAY_BUFFER))
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
    _vao = std::make_shared<GLVertexArray>();
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
    _meshDataCache[mesh] = data;
    return data;
  }
  OpenGLAPI::MaterialDesc::MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI * api) : _material(material)
  {
    _diffuseMap = api->createTexture(material->getDiffusePath());
    _normalMap = api->createTexture(material->getNormalPath());
    _alphaMap = api->createTexture(material->getOpacityPath());

    if (_diffuseMap && _alphaMap) {
      _materialSetup = std::make_shared<SetupAlphaMap>();
      _shader = api->_alphaShader;
    }
    else if (_diffuseMap) {
      _materialSetup = std::make_shared<SetupDiffuseMap>();
      _shader = api->_diffuseShader;
    }
    else {
      _materialSetup = std::make_shared<SetupDiffuseColor>();
      _shader = api->_colorShader;
    }
  }
  const std::shared_ptr<GLMaterialSetup>& OpenGLAPI::MaterialDesc::getMaterialSetup() const
  {
    return _materialSetup;
  }
  const std::shared_ptr<GLShaderProgram>& OpenGLAPI::MaterialDesc::getShader() const
  {
    return _shader;
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