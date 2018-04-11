#ifndef OPENGLAPI_H
#define OPENGLAPI_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <math/FlyMath.h>
#include <renderer/RenderParams.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <opengl/GLTexture.h>
#include <SoftwareCache.h>

namespace fly
{
  class GLBuffer;
  class GLVertexArray;
  class GLShaderProgram;
  class Mesh;
  class AABB;
  class GLAppendBuffer;
  class GLMaterialSetup;
  class Material;
  class GLFramebuffer;
  class GLSLShaderGenerator;
  struct Settings;
  struct GlobalShaderParams;

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
    using Shadowmap = GLTexture;
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
      std::unique_ptr < SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&>> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    class MaterialDesc
    {
    public:
      MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI* api, const Settings& settings);
      void create(OpenGLAPI* api, const Settings& settings);
      void create(const std::shared_ptr<Material>& material, OpenGLAPI* api, const Settings& settings);
      void setup() const;
      using ShaderProgram = GLShaderProgram;
      const std::shared_ptr<ShaderProgram>& getShader() const;
      const std::shared_ptr<ShaderProgram>& getSMShader() const;
      const std::shared_ptr<Material>& getMaterial() const;
    private:
      std::shared_ptr<Material> _material;
      std::vector<std::function<void()>> _shaderSetupFuncs;
      std::shared_ptr<ShaderProgram> _shader;
      std::shared_ptr<ShaderProgram> _smShader;
      std::shared_ptr<GLTexture> _diffuseMap;
      std::shared_ptr<GLTexture> _normalMap;
      std::shared_ptr<GLTexture> _alphaMap;
      std::shared_ptr<GLTexture> _heightMap;
    };
    void setupShader(GLShaderProgram* shader);
    void setupShader(GLShaderProgram* shader, const GlobalShaderParams& params);
    void setupShader(GLShaderProgram* shader, const GlobalShaderParams& param, const Depthbuffer* shadow_map);
    void setupMaterial(const MaterialDesc& desc, const GlobalShaderParams& param);
    void setupMaterial(const MaterialDesc& desc, const GlobalShaderParams& param, const Depthbuffer* shadow_map);
    void setupShaderConstants(const GlobalShaderParams& param);
    void setupShaderConstants(const GlobalShaderParams& param, const Depthbuffer* shadow_map);
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse);
    void renderMeshMVP(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mvp);
    void renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col);
    void setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer);
    void setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer);
    void bindBackbuffer(unsigned id) const;
    void composite(const RTT* lighting_buffer);
    std::shared_ptr<GLTexture> createTexture(const std::string& path);
    std::shared_ptr<MaterialDesc> createMaterial(const std::shared_ptr<Material>& material, const Settings& settings);
    std::shared_ptr<GLShaderProgram> createShader(const std::string& vertex_file, const std::string& fragment_file, const std::string& geometry_file = "");
    std::unique_ptr<RTT> createRenderToTexture(const Vec2u& size);
    std::unique_ptr<Depthbuffer> createDepthbuffer(const Vec2u& size);
    std::unique_ptr<Shadowmap> createShadowmap(const Vec2u& size, const Settings& settings);
    void recreateShadersAndMaterials(const Settings& settings);
    std::vector<std::shared_ptr<Material>> getAllMaterials();
  private:
    GLShaderProgram * _activeShader;
    SoftwareCache<std::string, std::shared_ptr<GLTexture>, const std::string& > _textureCache;
    SoftwareCache<std::string, std::shared_ptr<GLShaderProgram>, const std::string&, const std::string&, const std::string&> _shaderCache;
    SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>, const std::shared_ptr<Material>&, const Settings&> _matDescCache;
    std::shared_ptr<GLShaderProgram> _aabbShader;
    std::shared_ptr<GLShaderProgram> _compositeShader;
    std::shared_ptr<GLVertexArray> _vaoAABB;
    std::shared_ptr<GLBuffer> _vboAABB;
    std::unique_ptr<GLFramebuffer> _offScreenFramebuffer;
    std::unique_ptr<GLSLShaderGenerator> _shaderGenerator;

    void checkFramebufferStatus();
    void setColorBuffers(const std::vector<RTT*>& rtts);
  };
}

#endif
