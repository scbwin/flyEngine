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
    inline void setDepthEnabled() const
    {
      GL_CHECK(enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST));
    }
    struct MeshDesc
    {
      void* _indexOffset;
      unsigned _baseVertex;
      unsigned _numIndices;
      unsigned _materialIndex;
      Mesh* _mesh;
    };
    struct MaterialDesc
    {
      std::shared_ptr<GLTexture> _diffuseTexture;
      std::shared_ptr<GLTexture> _normalTexture;
      std::shared_ptr<GLTexture> _alphaTexture;
      Vec3f _diffuseColor;
    };
    struct ModelData
    {
      ModelData(const std::shared_ptr<Model>& model, OpenGLAPI* api);
      std::shared_ptr<GLVertexArray> _vao;
      std::shared_ptr<GLBuffer> _vbo;
      std::shared_ptr<GLBuffer> _ibo;
      std::vector<MeshDesc> _meshDesc; // For each mesh
      std::vector<MaterialDesc> _materialDesc; // For each material
    };
    // OpenGL wrapper for StaticModelRenderable
    struct StaticModelRenderable
    {
      std::vector<std::shared_ptr<ModelData>> _modelLods;
      std::shared_ptr<fly::StaticModelRenderable> _smr;
      AABB* getAABBWorld() const;
    };
    void renderModel(const StaticModelRenderable& smr, const Mat4f& mvp, unsigned lod) const;
  private:
    std::shared_ptr<GLShaderProgram> _simpleFullscreenQuadShader;
    std::shared_ptr<GLShaderProgram> _simpleShaderTextured;
    std::shared_ptr<GLShaderProgram> _simpleShaderColored;
    std::map<std::string, std::shared_ptr<GLTexture>> _textureCache;
    std::shared_ptr<GLTexture> createTexture(const std::string& path);
  };
}

#endif
