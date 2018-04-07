#ifndef OPENGLAPI_H
#define OPENGLAPI_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <math/FlyMath.h>
#include <renderer/RenderParams.h>
#include <memory>
#include <vector>
#include <unordered_map>

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
  class GLFramebuffer;

  class OpenGLAPI
  {
  public:
    OpenGLAPI();
    virtual ~OpenGLAPI();
    ZNearMapping getZNearMapping() const;
    void setViewport(const Vec2u& size) const;
    template<bool color, bool depth, bool stencil>
    void clearRendertarget(const Vec4f& clear_color) const
    {
      GLbitfield flag = 0;
      if (color) {
        GL_CHECK(glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]));
        flag |= GL_COLOR_BUFFER_BIT;
      }
      if (depth) {
        flag |= GL_DEPTH_BUFFER_BIT;
      }
      if (stencil) {
        flag |= GL_STENCIL_BUFFER_BIT;
      }
      GL_CHECK(glClear(flag));
    }
    static inline constexpr bool isDirectX() { return false; }
    template<bool enable>
    inline void setDepthTestEnabled() const { GL_CHECK(enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST)); }
    template<bool enable>
    inline void setFaceCullingEnabled() const { GL_CHECK(enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE)); }
    template<bool enable>
    inline void setDepthClampEnabled() const { GL_CHECK(enable ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP)); }
    void reloadShaders();
    using RTT = GLTexture;
    using Depthbuffer = GLTexture;
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
      std::unordered_map<std::shared_ptr<Mesh>, MeshData> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    class MaterialDesc
    {
    public:
      MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI* api);
      const std::unique_ptr<GLMaterialSetup>& getMaterialSetup() const;
      const std::shared_ptr<GLShaderProgram>& getShader() const;
      const std::shared_ptr<GLShaderProgram>& getSMShader() const;
      const std::shared_ptr<Material>& getMaterial() const;
      const std::shared_ptr<GLTexture>& getDiffuseMap() const;
      const std::shared_ptr<GLTexture>& getNormalMap() const;
      const std::shared_ptr<GLTexture>& getAlphaMap() const;
      using ShaderProgram = GLShaderProgram;
    private:
      std::shared_ptr<Material> _material;
      std::unique_ptr<GLMaterialSetup> _materialSetup;
      std::shared_ptr<ShaderProgram> _shader;
      std::shared_ptr<ShaderProgram> _smShader;
      std::shared_ptr<GLTexture> _diffuseMap;
      std::shared_ptr<GLTexture> _normalMap;
      std::shared_ptr<GLTexture> _alphaMap;
    };
    void setupShader(GLShaderProgram* shader);
    void setupShader(GLShaderProgram* shader, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity);
    void setupShader(GLShaderProgram* shader, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light);
    void setupMaterial(const MaterialDesc& desc);
    void setupMaterial(const MaterialDesc& desc, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity);
    void setupMaterial(const MaterialDesc& desc, const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light);
    void setupShaderConstants(const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity);
    void setupShaderConstants(const Vec3f& dl_pos_view_space, const Mat4f& projection_matrix, const Vec3f& light_intensity, const std::shared_ptr<Depthbuffer>& shadow_map, const Mat4f& view_to_light);
    void setupMaterialConstants(const std::shared_ptr<Material>& material);
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mv);
    void renderMeshMVP(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mvp);
    void renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col);
    void setRendertargets(const std::vector<std::shared_ptr<RTT>>& rtts, const std::shared_ptr<Depthbuffer>& depth_buffer);
    void bindBackbuffer(unsigned id) const;
    void composite(const std::shared_ptr<RTT>& lighting_buffer);
    std::shared_ptr<GLTexture> createTexture(const std::string& path);
    std::shared_ptr<MaterialDesc> createMaterial(const std::shared_ptr<Material>& material);
    std::shared_ptr<GLShaderProgram> createShader(const std::string& vertex_file, const std::string& fragment_file, const std::string& geometry_file = "");
    std::shared_ptr<RTT> createRenderToTexture(const Vec2u& size);
    std::shared_ptr<Depthbuffer> createDepthbuffer(const Vec2u& size);
  private:
    GLShaderProgram * _activeShader;
    std::unordered_map<std::string, std::shared_ptr<GLTexture>> _textureCache;
    std::unordered_map<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>> _matDescCache;
    std::unordered_map<std::string, std::shared_ptr<GLShaderProgram>> _shaderCache;
    std::shared_ptr<GLShaderProgram> _aabbShader;
    std::shared_ptr<GLShaderProgram> _compositeShader;
    std::shared_ptr<GLVertexArray> _vaoAABB;
    std::shared_ptr<GLBuffer> _vboAABB;
    std::unique_ptr<GLFramebuffer> _offScreenFramebuffer;
  };
}

#endif
