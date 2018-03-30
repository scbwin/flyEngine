#ifndef OPENGLAPI_H
#define OPENGLAPI_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <math/FlyMath.h>
#include <renderer/RenderParams.h>
#include <memory>
#include <vector>
#include <map>

namespace fly
{
  class GLBuffer;
  class GLVertexArray;
  class GLShaderProgram;
  class Mesh;
  class Model;
  class StaticModelRenderable;
  class GLTexture;
  class AABB;
  class GLAppendBuffer;

  class OpenGLAPI
  {
  public:
    OpenGLAPI();
    virtual ~OpenGLAPI() = default;
    ZNearMapping getZNearMapping() const;
    void initShaders();
    void setViewport(const Vec2u& size) const;
    void renderFullScreenQuad() const;
    void clearRendertargetColor(const Vec4f& color) const;
    static inline constexpr bool isDirectX() { return false; }
    template<bool enable>
    inline void setDepthTestEnabled() const
    {
      GL_CHECK(enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST));
    }
    using Texture = GLTexture;
    struct MeshDesc
    {
      GLsizei _numIndices;
      GLvoid* _indexOffset;
      GLint _baseVertex;
    };
    class MeshGeometryStorage
    {
    public:
      struct MeshData
      {
        size_t _count;
        GLvoid* _indices;
        GLint _baseVertex;
      };
      MeshGeometryStorage();
      void bind() const;
      MeshData addMesh(const std::shared_ptr<Mesh>& mesh);
    private:
      std::shared_ptr<GLVertexArray> _vao;
      std::shared_ptr<GLAppendBuffer> _vboAppend;
      std::shared_ptr<GLAppendBuffer> _iboAppend;
      std::map<std::shared_ptr<Mesh>, MeshData> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    void setupMaterial(const std::shared_ptr<Texture>& diffuse_tex, const Vec3f& diffuse_color);
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mvp);
    std::shared_ptr<GLTexture> createTexture(const std::string& path) const;
  private:
    std::shared_ptr<GLShaderProgram> _simpleFullscreenQuadShader;
    std::shared_ptr<GLShaderProgram> _simpleShaderTextured;
    std::shared_ptr<GLShaderProgram> _simpleShaderColored;
    std::shared_ptr<GLShaderProgram> _activeShader;
  };
}

#endif
