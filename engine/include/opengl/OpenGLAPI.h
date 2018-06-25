#ifndef OPENGLAPI_H
#define OPENGLAPI_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <math/FlyMath.h>
#include <ZNearMapping.h>
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
#include <opengl/GLBuffer.h>
#include <cstdint>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLMaterialSetup.h>

namespace fly
{
  class Mesh;
  class AABB;
  class Sphere;
  class Material;
  class GLSLShaderGenerator;
  struct GlobalShaderParams;
  class GLSampler;
  struct WindParamsLocal;
  class GraphicsSettings;

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
    using StorageBuffer = GLBuffer;
    using IndirectBuffer = GLBuffer;
    struct MeshData // For each mesh
    {
      GLsizei _count; // Number of indices (i.e. num triangles * 3)
      GLvoid* _indices; // Byte offset into the index buffer
      GLint _baseVertex; // Offset into the vertex buffer
      GLenum _type; // Either GL_UNSIGNED_SHORT or GL_UNSIGNED_INT, depending on the number of vertices of this mesh.
      inline unsigned numTriangles() const { return static_cast<unsigned>(_count / 3); }
    };
    /**
    * Geometry data for every single mesh that is added is stored in this structure.
    * There is one vertex array, one vertex buffer and one index buffer for the whole scene.
    * When rendering, only the vertex array has to be bound once at the beginning of the frame, 
    * this helps to keep state changes at a minimum.
    * TODO: remove data if a mesh gets destroyed.
    */
    class MeshGeometryStorage
    {
    public:
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
    struct IndirectInfo
    {
      unsigned _count; // Same as MeshData::_count
      unsigned _primCount; // Number of instances, typically reset each frame an then atomically incremented by a compute shader
      unsigned _firstIndex; // Corresponds to MeshData::_indices, but this is an index instead of a byte offset.
      unsigned _baseVertex; // Same as MeshData::_baseVertex
      unsigned _baseInstance; // Currently not used
      GLenum _type; // Same as MeshData::_type
      IndirectInfo(const MeshData& mesh_data)
      {
        _count = mesh_data._count;
        auto index = reinterpret_cast<std::uintptr_t>(mesh_data._indices) / (mesh_data._type == GL_UNSIGNED_SHORT ? sizeof(unsigned short) : sizeof(unsigned int));
        _firstIndex = static_cast<unsigned>(index);
        _baseVertex = mesh_data._baseVertex;
        _baseInstance = 0;
        _type = mesh_data._type;
      }
    };
    std::vector<IndirectInfo> indirectFromMeshData(const std::vector<MeshData>& mesh_data) const;
    // Texture unit bindings
    static constexpr const int diffuseTexUnit() { return 0; }
    static constexpr const int alphaTexUnit() { return 1; }
    static constexpr const int normalTexUnit() { return 2; }
    static constexpr const int heightTexUnit() { return 3; }
    static constexpr const int miscTexUnit0() { return 4; }
    static constexpr const int miscTexUnit1() { return 5; }
    static constexpr const int miscTexUnit2() { return 6; }
    static constexpr const int miscTexUnit3() { return 7; }
    static constexpr const int miscTexUnit4() { return 8; }
    void beginFrame() const;
    void bindShader(GLShaderProgram const * shader);
    void bindShadowmap(const Shadowmap& shadowmap) const;
    void renderMesh(const MeshData& mesh_data) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse, const WindParamsLocal& wind_params, const AABB& aabb) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const WindParamsLocal& wind_params, const Sphere& sphere) const;
    void renderMesh(const MeshData& mesh_data, const Mat4f& model_matrix, const Mat3f& model_matrix_inverse, const WindParamsLocal& wind_params, const Sphere& sphere) const;
    void renderMeshMVP(const MeshData& mesh_data, const Mat4f& mvp) const;
    void renderBVs(const StackPOD<AABB const *>& aabbs, const Mat4f& transform, const Vec3f& col);
    void renderBVs(const StackPOD<Sphere const *>& spheres, const Mat4f& transform, const Vec3f& col);
    void renderDebugFrustum(const Mat4f& vp_debug_frustum, const Mat4f& vp);
    void prepareCulling(const std::array<Vec4f, 6>& frustum_planes, const Vec3f& cam_pos_world, float lod_range, float thresh);
    void prepareLod(const Vec3f& cam_pos_world, float lod_range, float thresh);
    void endCulling() const;
    void cullInstances(const StorageBuffer& aabb_buffer, unsigned num_instances, const StorageBuffer& visible_instances, 
      const IndirectBuffer& indirect_draw_buffer, std::vector<IndirectInfo>& info) const;
    void renderInstances(const StorageBuffer& visible_instance_buffer, const IndirectBuffer& indirect_draw_buffer, const StorageBuffer& instance_data, 
      const std::vector<IndirectInfo>& info, unsigned num_instances) const;
    void setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer);
    void setRendertargets(const RendertargetStack& rtts, const Depthbuffer* depth_buffer, unsigned depth_buffer_layer);
    void bindBackbuffer(unsigned id) const;
    void ssr(const RTT& lighting_buffer, const RTT& view_space_normals, const Depthbuffer& depth_buffer, const Mat4f& projection_matrix, const Vec4f& blend_weight, RTT& lighting_buffer_copy);
    void separableBlur(const RTT& in, const std::array<std::shared_ptr<RTT>, 2>& out, RendertargetStack& rtt_stack);
    void renderGodRays(const Depthbuffer& depth_buffer, const RTT& lighting_buffer, const Vec2f& light_pos_uv);
    void composite(const RTT& lighting_buffer, const GlobalShaderParams& params);
    void composite(const RTT& lighting_buffer, const GlobalShaderParams& params, const RTT& dof_buffer, const Depthbuffer& depth_buffer);
    void composite(const RTT& lighting_buffer, const GlobalShaderParams& params, const RTT& dof_buffer, const Depthbuffer& depth_buffer, const RTT& god_ray_buffer, const Vec3f& god_ray_intensity);
    void endFrame() const;
    void setAnisotropy(unsigned anisotropy);
    void enablePolygonOffset(float factor, float units) const;
    void disablePolygonOffset() const;
    void renderSkydome(const Mat4f& view_projection_matrix, const MeshData& mesh_data);
    std::shared_ptr<Texture> createTexture(const std::string& path);
    std::shared_ptr<Shader> createShader(ShaderSource& vs, ShaderSource& fs, ShaderSource& gs = ShaderSource());
    Shader createComputeShader(ShaderSource& source);
    std::unique_ptr<RTT> createRenderToTexture(const Vec2u& size, TexFilter filter);
    std::unique_ptr<Depthbuffer> createDepthbuffer(const Vec2u& size);
    std::unique_ptr<Shadowmap> createShadowmap(const GraphicsSettings& settings);
    template<typename T>
    StorageBuffer createStorageBuffer(T const* data, size_t num_elements) const
    {
      StorageBuffer buffer(GL_SHADER_STORAGE_BUFFER);
      buffer.setData(data, num_elements);
      return buffer;
    }
    IndirectBuffer createIndirectBuffer(const IndirectInfo& info) const;
    IndirectBuffer createIndirectBuffer(const std::vector<IndirectInfo>& info) const;
    void resizeShadowmap(Shadowmap* shadow_map, const GraphicsSettings& settings);
    void createBlurShader(const GraphicsSettings& gs);
    void createCompositeShader(const GraphicsSettings& gs);
    void createScreenSpaceReflectionsShader(const GraphicsSettings& gs);
    void createGodRayShader(const GraphicsSettings& gs);
    const ShaderGenerator& getShaderGenerator() const;
    Shader const *& getActiveShader();
  private:
    struct GlewInit
    {
      GlewInit();
    };
    void checkFramebufferStatus();
    void setColorBuffers(const RendertargetStack & rtts);
    GlewInit _glewInit;
    GLShaderProgram const * _activeShader;
    GLFramebuffer _offScreenFramebuffer;
    StackPOD<GLenum> _drawBuffers;
    Shader _compositeShader;
    Shader _skydomeShader;
    GLShaderProgram _ssrShader;
    GLShaderProgram _blurShader;
    GLShaderProgram _boxShader;
    GLShaderProgram _cullingShader;
    GLShaderProgram _lodShader;
    GLVertexArray _vaoAABB;
    GLBuffer _vboAABB;
    ShaderGenerator _shaderGenerator;
    GLSampler _samplerAnisotropic;
    GLint _glVersionMajor, _glVersionMinor;
    Shader _debugFrustumShader;
    Shader _godRayShader;
    unsigned _anisotropy = 1u;
    inline void activateTexture(const Texture& texture, const char* key, GLint tex_unit) const
    {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + tex_unit));
      texture.bind();
      setScalar(_activeShader->uniformLocation(key), tex_unit);
    }
    Shader createMiscShader(GLShaderSource& vs, GLShaderSource& fs) const;
  };
}

#endif
