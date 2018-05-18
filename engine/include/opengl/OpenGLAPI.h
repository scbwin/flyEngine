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
#include <opengl/GLVertexArray.h>
#include <opengl/GLByteBufferStack.h>
#include <opengl/GLShaderSource.h>
#include <StackPOD.h>
#include <opengl/GLMaterialSetup.h>
#include <opengl/GLShaderSetup.h>
#include <opengl/GLSLShaderGenerator.h>
#include <opengl/GLShaderProgram.h>
#include <opengl/GLFramebuffer.h>
#include <opengl/GLSampler.h>

namespace fly
{
  class GLBuffer;
  class Mesh;
  class AABB;
  class Material;
  class GLSLShaderGenerator;
  struct GlobalShaderParams;
  class GLSampler;
  struct WindParamsLocal;
  class GraphicsSettings;
  template<typename API>
  class ShaderDesc;

  class OpenGLAPI
  {
  public:
    OpenGLAPI(const Vec4f& clear_color);
    ~OpenGLAPI();
    ZNearMapping getZNearMapping() const;
    void setViewport(const Vec2u& size) const;
    enum class TexFilter
    {
      NEAREST, LINEAR
    };
    void clearRendertarget(bool color, bool depth, bool stencil) const
    {
      GLbitfield flag = 0;
      if (color) {
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
    static const size_t _maxRendertargets = 8;
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
    enum class CullMode{BACK, FRONT};
    template<CullMode m> inline void setCullMode() const
    {
      GL_CHECK(glCullFace(m == CullMode::BACK ? GL_BACK : GL_FRONT));
    }
    void reloadShaders();
    using RTT = GLTexture;
    using Depthbuffer = GLTexture;
    using Shadowmap = GLTexture;
    using Texture = GLTexture;
    using Shader = GLShaderProgram;
    using RendertargetStack = StackPOD<RTT const *>;
    using ShaderGenerator = GLSLShaderGenerator;
    using MaterialSetup = GLMaterialSetup;
    using ShaderSetup = GLShaderSetup;
    using ShaderSource = GLShaderSource;
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
        inline unsigned numTriangles() const {return static_cast<unsigned>(_count / 3);}
        GLenum _type;
      };
      MeshGeometryStorage();
      ~MeshGeometryStorage();
      void bind() const;
      MeshData addMesh(const std::shared_ptr<Mesh>& mesh);
    private:
      GLVertexArray _vao;
      GLByteBufferStack _vboStack;
      GLByteBufferStack _iboStack;
      SoftwareCache<std::shared_ptr<Mesh>, MeshData, const std::shared_ptr<Mesh>&> _meshDataCache;
      size_t _indices = 0;
      size_t _baseVertex = 0;
    };
    // Texture unit bindings
    static constexpr const int diffuseTexUnit() { return 0; }
    static constexpr const int alphaTexUnit() { return 1; }
    static constexpr const int normalTexUnit() { return 2; }
    static constexpr const int heightTexUnit() { return 3; }
    static constexpr const int miscTexUnit0() { return 4; }
    static constexpr const int miscTexUnit1() { return 5; }
    static constexpr const int miscTexUnit2() { return 6; }
    static constexpr const int miscTexUnit3() { return 7; }
    void beginFrame() const;
    void bindShader(GLShaderProgram* shader);
    void bindShadowmap(const Shadowmap& shadowmap) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse, const Mat3f& model_view_inverse) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMesh(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMeshMVP(const MeshGeometryStorage::MeshData& mesh_data, const Mat4f& mvp) const;
    void renderAABBs(const std::vector<AABB const *>& aabbs, const Mat4f& transform, const Vec3f& col);
    void setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer);
    void setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer);
    void bindBackbuffer(unsigned id) const;
    void ssr(const RTT& lighting_buffer, const RTT& view_space_normals, const Depthbuffer& depth_buffer, const Mat4f& projection_matrix, const Vec4f& blend_weight, RTT& lighting_buffer_copy);
    void separableBlur(const RTT& in, const std::array<std::shared_ptr<RTT>, 2>& out, RendertargetStack& rtt_stack);
    void composite(const RTT& lighting_buffer, const GlobalShaderParams& params);
    void composite(const RTT& lighting_buffer, const GlobalShaderParams& params, const RTT& dof_buffer, const Depthbuffer& depth_buffer);
    void endFrame() const;
    void setAnisotropy(unsigned anisotropy);
    std::shared_ptr<OpenGLAPI::Texture> createTexture(const std::string& path);
    std::shared_ptr<GLShaderProgram> createShader(GLShaderSource& vs, GLShaderSource& fs, GLShaderSource& gs = GLShaderSource());
    std::unique_ptr<RTT> createRenderToTexture(const Vec2u& size, TexFilter filter);
    std::unique_ptr<Depthbuffer> createDepthbuffer(const Vec2u& size);
    std::unique_ptr<Shadowmap> createShadowmap(const GraphicsSettings& settings);
    void resizeShadowmap(Shadowmap* shadow_map, const GraphicsSettings& settings);
    void createBlurShader(const GraphicsSettings& gs);
    void createCompositeShader(const GraphicsSettings& gs);
    void createScreenSpaceReflectionsShader(const GraphicsSettings& gs);
    const std::unique_ptr<ShaderDesc<OpenGLAPI>>& getSkyboxShaderDesc() const;
    const ShaderGenerator& getShaderGenerator() const;
    Shader*& getActiveShader();
  private:
    struct GlewInit
    {
      GlewInit();
    };
    void checkFramebufferStatus();
    void setColorBuffers(const RendertargetStack & rtts);
    GlewInit _glewInit;
    GLShaderProgram * _activeShader;
    GLFramebuffer _offScreenFramebuffer;
    StackPOD<GLenum> _drawBuffers;
    std::unique_ptr<ShaderDesc<OpenGLAPI>> _compositeShaderDesc;
    std::unique_ptr<ShaderDesc<OpenGLAPI>> _skydomeShaderDesc;
    GLShaderProgram _ssrShader;
    GLShaderProgram _blurShader;
    GLShaderProgram _aabbShader;
    GLVertexArray _vaoAABB;
    GLBuffer _vboAABB;
    ShaderGenerator _shaderGenerator;
    GLSampler _samplerAnisotropic;
    GLint _glVersionMajor, _glVersionMinor;
    unsigned _anisotropy = 1u;
  };
}

#endif
