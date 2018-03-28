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

    _simpleShaderTextured = std::make_shared<GLShaderProgram>();
    _simpleShaderTextured->create();
    _simpleShaderTextured->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _simpleShaderTextured->addShaderFromFile("assets/opengl/fs_simple_textured.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _simpleShaderTextured->link();

    _simpleShaderColored = std::make_shared<GLShaderProgram>();
    _simpleShaderColored->create();
    _simpleShaderColored->addShaderFromFile("assets/opengl/vs_simple.glsl", GLShaderProgram::ShaderType::VERTEX);
    _simpleShaderColored->addShaderFromFile("assets/opengl/fs_simple_color.glsl", GLShaderProgram::ShaderType::FRAGMENT);
    _simpleShaderColored->link();
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
  void OpenGLAPI::bindGeometryData(const GeometryData & data)
  {
    data._vao->bind();
  }
  void OpenGLAPI::setupMaterial(const std::shared_ptr<Texture>& diffuse_tex, const Vec3f & diffuse_color)
  {
    _activeShader = diffuse_tex ? _simpleShaderTextured : _simpleShaderColored;
    _activeShader->bind();
    if (diffuse_tex) {
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      diffuse_tex->bind();
      GL_CHECK(glUniform1i(_activeShader->uniformLocation("ts"), 0));
    }
    else {
      GL_CHECK(glUniform3f(_activeShader->uniformLocation("color"), diffuse_color[0], diffuse_color[1], diffuse_color[2]));
    }
  }
  void OpenGLAPI::renderMesh(const MeshDesc& mesh_desc, const Mat4f& mvp) const
  {
    GL_CHECK(glUniformMatrix4fv(_activeShader->uniformLocation("MVP"), 1, false, mvp.ptr()));
    GL_CHECK(glDrawElementsBaseVertex(GL_TRIANGLES, mesh_desc._numIndices, GL_UNSIGNED_INT, mesh_desc._indexOffset, mesh_desc._baseVertex));
  }
  void OpenGLAPI::renderMeshBatch(const BatchDesc & batch_desc, const Mat4f & mvp) const
  {
    GL_CHECK(glUniformMatrix4fv(_activeShader->uniformLocation("MVP"), 1, false, mvp.ptr()));
    GL_CHECK(glMultiDrawElementsBaseVertex(GL_TRIANGLES, const_cast<GLsizei*>(batch_desc._count.data()), GL_UNSIGNED_INT,
      const_cast<GLvoid**>(batch_desc._indices.data()), static_cast<GLsizei>(batch_desc._count.size()), const_cast<GLint*>(batch_desc._baseVertex.data())));
  }
  std::shared_ptr<GLTexture> OpenGLAPI::createTexture(const std::string & path) const
  {
    if (path == "") {
      return nullptr;
    }
    auto tex = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_COMPRESS_TO_DXT);
    if (tex == 0) {
      return nullptr;
    }
    return std::make_shared<GLTexture>(tex, GL_TEXTURE_2D);
  }
  OpenGLAPI::GeometryData::GeometryData(const std::vector<std::shared_ptr<Mesh>>& meshes, OpenGLAPI * api, std::vector<OpenGLAPI::MeshDesc>& mesh_descs)
  {
    mesh_descs.resize(meshes.size());
    std::vector<Vertex> vertices;
    std::vector<unsigned> indices;
    for (unsigned i = 0; i < meshes.size(); i++) {
      const auto& m = meshes[i];
      mesh_descs[i]._baseVertex = static_cast<GLint>(vertices.size());
      mesh_descs[i]._indexOffset = reinterpret_cast<GLvoid*>(indices.size() * sizeof(indices.front()));
      mesh_descs[i]._numIndices = static_cast<GLsizei>(m->getIndices().size());
      vertices.insert(vertices.end(), m->getVertices().begin(), m->getVertices().end());
      indices.insert(indices.end(), m->getIndices().begin(), m->getIndices().end());
    }
    _vao = std::make_shared<GLVertexArray>();
    _vao->bind();
    _vbo = std::make_shared<GLBuffer>(GL_ARRAY_BUFFER);
    _vbo->setData(&vertices.front(), vertices.size());
    _ibo = std::make_shared<GLBuffer>(GL_ELEMENT_ARRAY_BUFFER);
    _ibo->setData(&indices.front(), indices.size());
    for (unsigned i = 0; i < 5; i++) {
      GL_CHECK(glEnableVertexAttribArray(i));
    }
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _position))));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _normal))));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _uv))));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _tangent))));
    GL_CHECK(glVertexAttribPointer(4, 3, GL_FLOAT, false, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _bitangent))));
  }
  void OpenGLAPI::BatchDesc::addMeshDesc(const MeshDesc & mesh_desc)
  {
    _count.push_back(mesh_desc._numIndices);
    _indices.push_back(mesh_desc._indexOffset);
    _baseVertex.push_back(mesh_desc._baseVertex);
  }
}