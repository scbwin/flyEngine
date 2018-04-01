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
  class GLTexture;
  class AABB;
  class GLAppendBuffer;
  class GLMaterialSetup;
  class Material;

  class OpenGLAPI
  {
  public:
    OpenGLAPI();
    virtual ~OpenGLAPI() = default;
    ZNearMapping getZNearMapping() const;
    void setViewport(const Vec2u& size) const;
    void clearRendertargetColor(const Vec4f& color) const;
    static inline constexpr bool isDirectX() { return false; }
    template<bool enable>
    inline void setDepthTestEnabled() const
    {
      GL_CHECK(enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST));
    }
    class MeshGeometryStorage
    {
    public:
      struct MeshData
      {
        GLsizei _count;
        GLvoid* _indices;
        GLint _baseVertex;
      };
      MeshGeometryStorage();
      ~MeshGeometryStorage();
      void bind() const;
      MeshData addMesh(const std::shared_ptr<Mesh>& mesh);
    private:
      std::unique_ptr<GLVertexArray> _vao;
      std::unique_ptr<GLAppendBuffer> _vboAppend;
      std::unique_ptr<GLAppendBuffer> _iboAppend;
      std::map<std::shared_ptr<Mesh>, MeshData> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    class MaterialDesc
    {
    public:
      MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI* api);
      const std::unique_ptr<GLMaterialSetup>& getMaterialSetup() const;
      const std::shared_ptr<GLShaderProgram>& getShader() const;
      const std::shared_ptr<Material>& getMaterial() const;
      const std::shared_ptr<GLTexture>& getDiffuseMap() const;
      const std::shared_ptr<GLTexture>& getNormalMap() const;
      const std::shared_ptr<GLTexture>& getAlphaMap() const;
      using ShaderProgram = GLShaderProgram;
    private:
      std::shared_ptr<Material> _material;
      std::unique_ptr<GLMaterialSetup> _materialSetup;
      std::shared_ptr<ShaderProgram> _shader;
      std::shared_ptr<GLTexture> _diffuseMap;
      std::shared_ptr<GLTexture> _normalMap;
      std::shared_ptr<GLTexture> _alphaMap;
    };
    void setupShader(GLShaderProgram* shader, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix);
    void setupMaterial(const MaterialDesc& desc);
    void setupMaterial(const MaterialDesc& desc, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix);
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mv);
    void renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col);
    std::shared_ptr<GLTexture> createTexture(const std::string& path);
    std::shared_ptr<MaterialDesc> createMaterial(const std::shared_ptr<Material>& material);
    std::shared_ptr<GLShaderProgram> createShader(const std::string& vertex_file, const std::string& fragment_file, const std::string& geometry_file = "");
  private:
    GLShaderProgram* _activeShader;
    std::map<std::string, std::shared_ptr<GLTexture>> _textureCache;
    std::map<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>> _matDescCache;
    std::map<std::string, std::shared_ptr<GLShaderProgram>> _shaderCache;
    std::shared_ptr<GLShaderProgram> _aabbShader;
    std::shared_ptr<GLVertexArray> _vaoAABB;
    std::shared_ptr<GLBuffer> _vboAABB;
  };
}

#endif
