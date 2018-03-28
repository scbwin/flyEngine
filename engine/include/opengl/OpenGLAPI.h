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
    struct BatchDesc
    {
      std::vector<GLsizei> _count; // Primitive counts
      std::vector<GLvoid*> _indices; // Pointers into the index buffer
      std::vector<GLint> _baseVertex; // Offset that is added to the indices
      void addMeshDesc(const MeshDesc& mesh_desc);
    };
    struct GeometryData
    {
      GeometryData(const std::vector<std::shared_ptr<Mesh>>& meshes, OpenGLAPI* api, std::vector<MeshDesc>& mesh_descs);
      std::shared_ptr<GLVertexArray> _vao;
      std::shared_ptr<GLBuffer> _vbo;
      std::shared_ptr<GLBuffer> _ibo;
    };
    void bindGeometryData(const GeometryData& data);
    void setupMaterial(const std::shared_ptr<Texture>& diffuse_tex, const Vec3f& diffuse_color);
    void renderMesh(const MeshDesc& mesh_desc, const Mat4f& mvp) const;
    void renderMeshBatch(const BatchDesc& batch_desc, const Mat4f& mvp) const;
    std::shared_ptr<GLTexture> createTexture(const std::string& path) const;
  private:
    std::shared_ptr<GLShaderProgram> _simpleFullscreenQuadShader;
    std::shared_ptr<GLShaderProgram> _simpleShaderTextured;
    std::shared_ptr<GLShaderProgram> _simpleShaderColored;
    std::shared_ptr<GLShaderProgram> _activeShader;
  };
}

#endif
