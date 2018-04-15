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
  struct GlobalShaderParams;
  class GLSampler;
  struct WindParamsLocal;
  class GraphicsSettings;

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
    template<bool enable> inline void setDepthTestEnabled() const { GL_CHECK(enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST)); }
    template<bool enable> inline void setFaceCullingEnabled() const { GL_CHECK(enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE)); }
    template<bool enable> inline void setDepthClampEnabled() const { GL_CHECK(enable ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP)); }
    template<bool enable> inline void setDepthWriteEnabled() const { GL_CHECK(glDepthMask(enable)); }
    enum class DepthFunc { NEVER, LESS, EQUAL, LEQUAL, GREATER, NOTEQUAL, GEQUAL, ALWAYS };
    template<DepthFunc f> inline void setDepthFunc() const
    {
      GLenum func;
      if (f == DepthFunc::NEVER) {
        func = GL_NEVER;
      }
      else if (f == DepthFunc::LESS) {
        func = GL_LESS;
      }
      else if (f == DepthFunc::EQUAL) {
        func = GL_EQUAL;
      }
      else if (f == DepthFunc::LEQUAL) {
        func = GL_LEQUAL;
      }
      else if (f == DepthFunc::GREATER) {
        func = GL_GREATER;
      }
      else if (f == DepthFunc::NOTEQUAL) {
        func = GL_NOTEQUAL;
      }
      else if (f == DepthFunc::GEQUAL) {
        func = GL_GEQUAL;
      }
      else if (f == DepthFunc::ALWAYS) {
        func = GL_ALWAYS;
      }
      GL_CHECK(glDepthFunc(func));
    }
    void reloadShaders();
    using RTT = GLTexture;
    using Depthbuffer = GLTexture;
    using Shadowmap = GLTexture;
    /**
    * Geometry data for every single mesh that is added is stored in this structure.
    * There is one vertex array, one big vertex buffer and one big index buffer for the whole scene.
    * When rendering, only the vertex array has to be bound once at the beginning of the frame, 
    * this helps to keep state changes at a minimum.
    */
    class MeshGeometryStorage
    {
    public:
      struct MeshData // For each mesh
      {
        GLsizei _count; // Number of indices (i.e. num triangles * 3)
        GLvoid* _indices; // Byte offset into the index buffer
        GLint _baseVertex; // Offset into the vertex buffer
      };
      MeshGeometryStorage();
      ~MeshGeometryStorage();
      void bind() const;
      MeshData addMesh(const std::shared_ptr<Mesh>& mesh);
    private:
      std::unique_ptr<GLVertexArray> _vao;
      std::unique_ptr<GLAppendBuffer> _vboAppend;
      std::unique_ptr<GLAppendBuffer> _iboAppend;
      SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    // Texture unit bindings
    static constexpr const int diffuseTexUnit() { return 0; }
    static constexpr const int alphaTexUnit() { return 1; }
    static constexpr const int normalTexUnit() { return 2; }
    static constexpr const int heightTexUnit() { return 3; }
    static constexpr const int shadowTexUnit() { return 4; }
    static constexpr const int lightingTexUnit() { return 5; }
    enum ShaderSetupFlags : unsigned
    {
      NONE = 0,
      SHADOWS = 1,
      TIME = 2,
      WIND = 4,
      VP = 8,
      LIGHTING = 16,
      EXPOSURE = 32
    };
    /**
    * Class that is used to only send uniform data to the GPU that is actually needed.
    */
    class ShaderDesc
    {
    public:
      ShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, unsigned flags);
      void setup(const GlobalShaderParams& params) const;
      const std::shared_ptr<GLShaderProgram>& getShader() const;
    private:
      std::shared_ptr<GLShaderProgram> _shader;
      std::vector<std::function<void(const GlobalShaderParams&)>> _setupFuncs;
    };
    class MaterialDesc
    {
    public:
      MaterialDesc(const std::shared_ptr<Material>& material, OpenGLAPI* api, const GraphicsSettings& settings);
      void create(OpenGLAPI* api, const GraphicsSettings& settings);
      void create(const std::shared_ptr<Material>& material, OpenGLAPI* api, const GraphicsSettings& settings);
      void setup(GLShaderProgram* shader) const;
      void setupDepth(GLShaderProgram* shader) const;
      using ShaderProgram = GLShaderProgram;
      const std::shared_ptr<ShaderDesc>& getMeshShaderDesc(bool has_wind) const;
      const std::shared_ptr<ShaderDesc>& getMeshShaderDescDepth(bool has_wind) const;
      const std::shared_ptr<Material>& getMaterial() const;
    private:
      std::shared_ptr<Material> _material;
      std::vector<std::function<void(GLShaderProgram*)>> _materialSetupFuncs;
      std::vector<std::function<void(GLShaderProgram*)>> _materialSetupFuncsDepth;
      std::shared_ptr<ShaderDesc> _meshShaderDesc;
      std::shared_ptr<ShaderDesc> _meshShaderDescWind;
      std::shared_ptr<ShaderDesc> _meshShaderDescDepth;
      std::shared_ptr<ShaderDesc> _meshShaderDescWindDepth;
      std::shared_ptr<GLTexture> _diffuseMap;
      std::shared_ptr<GLTexture> _normalMap;
      std::shared_ptr<GLTexture> _alphaMap;
      std::shared_ptr<GLTexture> _heightMap;
    };
    void beginFrame() const;
    void bindShader(GLShaderProgram* shader);
    void setupShaderDesc(const ShaderDesc& desc, const GlobalShaderParams& params);
    void bindShadowmap(const Shadowmap& shadowmap) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMeshMVP(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mvp) const;
    void renderAABBs(const std::vector<AABB*>& aabbs, const Mat4f& transform, const Vec3f& col);
    void setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer);
    void setRendertargets(const std::vector<RTT*>& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer);
    void bindBackbuffer(unsigned id) const;
    void composite(const RTT* lighting_buffer, const GlobalShaderParams& params);
    void endFrame() const;
    void setAnisotropy(unsigned anisotropy);
    std::shared_ptr<GLTexture> createTexture(const std::string& path);
    std::shared_ptr<MaterialDesc> createMaterial(const std::shared_ptr<Material>& material, const GraphicsSettings& settings);
    std::shared_ptr<GLShaderProgram> createShader(const std::string& vertex_file, const std::string& fragment_file, const std::string& geometry_file = "");
    std::shared_ptr<ShaderDesc> createShaderDesc(const std::shared_ptr<GLShaderProgram>& shader, unsigned flags);
    std::unique_ptr<RTT> createRenderToTexture(const Vec2u& size);
    std::unique_ptr<Depthbuffer> createDepthbuffer(const Vec2u& size);
    std::unique_ptr<Shadowmap> createShadowmap(const Vec2u& size, const GraphicsSettings& settings);
    void recreateShadersAndMaterials(const GraphicsSettings& gs);
    void createCompositeShaderFile(const GraphicsSettings& gs);
    std::vector<std::shared_ptr<Material>> getAllMaterials();
  private:
    GLShaderProgram * _activeShader;
    SoftwareCache<std::string, std::shared_ptr<GLTexture>, const std::string& > _textureCache;
    SoftwareCache<std::string, std::shared_ptr<GLShaderProgram>, const std::string&, const std::string&, const std::string&> _shaderCache;
    SoftwareCache<std::shared_ptr<Material>, std::shared_ptr<MaterialDesc>, const std::shared_ptr<Material>&, const GraphicsSettings&> _matDescCache;
    SoftwareCache<std::shared_ptr<GLShaderProgram>, std::shared_ptr<ShaderDesc>, const std::shared_ptr<GLShaderProgram>&, unsigned> _shaderDescCache;
    std::shared_ptr<GLShaderProgram> _aabbShader;
    std::shared_ptr<ShaderDesc> _compositeShaderDesc;
    std::shared_ptr<GLVertexArray> _vaoAABB;
    std::shared_ptr<GLBuffer> _vboAABB;
    std::unique_ptr<GLFramebuffer> _offScreenFramebuffer;
    std::unique_ptr<GLSLShaderGenerator> _shaderGenerator;
    std::unique_ptr<GLSampler> _samplerAnisotropic;
    GLint _glVersionMajor, _glVersionMinor;
    unsigned _anisotropy = 1u;

    void checkFramebufferStatus();
    void setColorBuffers(const std::vector<RTT*>& rtts);
  };
}

#endif
