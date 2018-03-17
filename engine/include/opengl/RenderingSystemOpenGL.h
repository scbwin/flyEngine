#ifndef OPENGLINTERFACE_H
#define OPENGLINTERFACE_H

#include "RenderingSystem.h"
#include "GL/glew.h"
#include "opengl/GLWrappers.h"
#include <map>
#include <mutex>
#include <future>
#include <Terrain.h>

#define NONE 0
#define NORTH 1
#define SOUTH 2
#define WEST 4
#define EAST 8

#define PROFILE 0

namespace fly
{
  class Camera;
  class Transform;

  class RenderingSystemOpenGL : public RenderingSystem
  {
  public:
    RenderingSystemOpenGL();
    virtual void init(const Vec2i& window_size) override;
    virtual ~RenderingSystemOpenGL();
    virtual void setSkybox(const std::array<std::string, 6u>& paths) override;
    virtual void setSkydome(const std::shared_ptr<Mesh>& mesh) override;
    virtual void update(float time, float delta_time) override;
    virtual void initShaders() override;
    virtual void onDirectionalLightAdded(Entity* entity, bool always_create) override;
    virtual void onSpotLightAdded(Entity* entity, bool always_createl) override;
    virtual void onPointLightAdded(Entity* entity, bool always_create) override;
    virtual void onLightRemoved(Entity* entity) override;
    virtual void onTerrainAdded(Entity* entity) override;
    virtual void onTerrainRemoved(Entity* entity) override;
    virtual void onResize(const glm::ivec2& size) override;
    virtual float getSceneDepth(const glm::ivec2& pos) override;
    void setDefaultFramebufferId(unsigned int fb_id);
    void getSobelKernel(std::vector<float>& smooth, std::vector<float>& gradient);

    bool _useTreeBillboards = false;

#if PROFILE
    class Timing
    {
    public:
      Timing()
      {
        GL_CHECK(glFinish());
        _start = std::chrono::high_resolution_clock::now();
      }

      void stop()
      {
        GL_CHECK(glFinish());
        auto end = std::chrono::high_resolution_clock::now();
        _elapsedMillis = std::chrono::duration_cast<std::chrono::nanoseconds>(end - _start).count() / 1000000.0f;
      }

      float getElapsedMillis()
      {
        return _elapsedMillis;
      }

    private:
      std::chrono::time_point<std::chrono::high_resolution_clock> _start;
      float _elapsedMillis;
    };

    unsigned int getTreeDrawCalls();
    unsigned int getTreeTriangles();
    unsigned int getTreeTrianglesShadowMap();
    unsigned int getGrassDrawCalls();
    unsigned int getGrassBlades();
    unsigned int getGrassTriangles();
    Timing getTiming(RenderStage stage);
#endif

  private:
    std::shared_ptr<GLShaderProgram> _shaderProgramDepth;
    std::shared_ptr<GLShaderProgram> _shaderProgramDepthLayered;
    std::shared_ptr<GLShaderProgram> _shaderProgramDepthPointLight;
    std::shared_ptr<GLShaderProgram> _shaderProgramScreen;
    std::shared_ptr<GLShaderProgram> _shaderProgramGodRay;
    std::shared_ptr<GLShaderProgram> _shaderProgramDeferredGeometryTextured;
    std::shared_ptr<GLShaderProgram> _shaderProgramDeferredLightingDirectional;
    std::shared_ptr<GLShaderProgram> _shaderProgramDeferredLightingSpot;
    std::shared_ptr<GLShaderProgram> _shaderProgramDeferredLightingPoint;
    std::shared_ptr<GLShaderProgram> _shaderProgramPingPongFilter;
    std::shared_ptr<GLShaderProgram> _shaderProgramPingPongFilterBilateral;
    std::shared_ptr<GLShaderProgram> _shaderProgramBloom;
    std::shared_ptr<GLShaderProgram> _shaderProgramMotionBlur;
    std::shared_ptr<GLShaderProgram> _shaderProgramLightGather;
    std::shared_ptr<GLShaderProgram> _shaderProgramBrightness;
    std::shared_ptr<GLShaderProgram> _shaderProgramSkybox;
    std::shared_ptr<GLShaderProgram> _shaderProgramWaterForward;
    std::shared_ptr<GLShaderProgram> _shaderProgramUnderwater;
    std::shared_ptr<GLShaderProgram> _shaderProgramWaterPopupDistortion;
    std::shared_ptr<GLShaderProgram> _shaderProgramExposure;
    std::shared_ptr<GLShaderProgram> _shaderProgramVolumeLight;
    std::shared_ptr<GLShaderProgram> _shaderProgramVolumeLightSpot;
    std::shared_ptr<GLShaderProgram> _shaderProgramVolumeLightPoint;
    std::shared_ptr<GLShaderProgram> _shaderProgramGradient;
    std::shared_ptr<GLShaderProgram> _shaderProgramDOF;
    std::shared_ptr<GLShaderProgram> _shaderProgramDOFGather;
    std::shared_ptr<GLShaderProgram> _shaderProgramDOFRefDepth;
    std::shared_ptr<GLShaderProgram> _aabbShader;
    std::shared_ptr<GLShaderProgram> _lensFlareShader;
    std::shared_ptr<GLShaderProgram> _lensFlareCombineShader;
    std::shared_ptr<GLShaderProgram> _skydomeShader;
    std::shared_ptr<GLShaderProgram> _ssrShader;
    std::shared_ptr<GLShaderProgram> _terrainShader;
    std::shared_ptr<GLShaderProgram> _terrainWireframeShader;
    std::shared_ptr<GLShaderProgram> _normalMapShader;
    std::shared_ptr<GLShaderProgram> _grassShader;
    std::shared_ptr<GLShaderProgram> _treeShader;
    std::shared_ptr<GLShaderProgram> _impostorShader;
    std::shared_ptr<GLShaderProgram> _treeBillboardShader;
    std::shared_ptr<GLShaderProgram> _treeShadowMapShader;
    std::shared_ptr<GLShaderProgram> _leavesShader;
    std::shared_ptr<GLShaderProgram> _leavesShadowMapShader;
    std::shared_ptr<GLShaderProgram> _leavesImpostorShader;
    std::shared_ptr<GLShaderProgram> _cloudBillboardShader;
    std::shared_ptr<GLShaderProgram> _waterShader;
    std::shared_ptr<GLShaderProgram> _directionalLightShader;
    std::shared_ptr<GLShaderProgram> _waterPopUpDistortionShader;
    std::shared_ptr<GLShaderProgram> _underwaterShader;

    std::array<std::shared_ptr<GLFramebuffer>, 2> _framebufferQuarterSize;
    std::array<std::shared_ptr<GLFramebuffer>, 2> _gradientBufferXQuarterSize;
    std::array<std::shared_ptr<GLFramebuffer>, 2> _gradientBufferYQuarterSize;
    std::array<std::shared_ptr<GLFramebuffer>, 2> _DOFBlurBuffer;
    std::array<std::shared_ptr<GLFramebuffer>, 2> _focusBuffer;
    std::shared_ptr<GLFramebuffer> _gradientBufferQuarterSize;

    std::array<std::shared_ptr<GLFramebuffer>, 2> _lightingBuffer;

    std::vector<std::array<std::shared_ptr<GLFramebuffer>, 2>> _bloomFramebuffers;

    std::shared_ptr<GLTexture> _sceneDepthbuffer;

    unsigned int _defaultFramebufferId = 0;

    std::map<std::string, std::shared_ptr<GLTexture>> _textures;
    std::shared_ptr<GLFramebuffer> _gBuffer; // used for deferred shading

    std::map<Entity*, std::shared_ptr<GLFramebuffer>> _shadowMaps;
    std::map<Entity*, std::shared_ptr<GLQuery>> _lightQueries;

    unsigned int _frameCount = 0;

#if PROFILE
    std::map<RenderStage, Timing> _timings;
    unsigned int _treeDrawCalls;
    unsigned int _treeTriangles;
    unsigned int _treeTrianglesShadowMap;
    unsigned int _grassDrawCalls;
    unsigned int _grassBlades;
#endif

    glm::vec3 _camPos;

    enum class TerrainRenderMode
    {
      NORMAL, WIREFRAME, GRASS
    };

    struct MeshBinding
    {
      MeshBinding(const std::shared_ptr<Mesh>& mesh);
      std::shared_ptr<GLVertexArray> _vertexArray;
      std::shared_ptr<GLBuffer> _vertexBuffer;
      std::shared_ptr<GLBuffer> _indexBuffer;
    };
    std::map<std::shared_ptr<Mesh>, std::shared_ptr<MeshBinding>> _meshBindings;


    struct TerrainRenderable
    {
      TerrainRenderable(Entity* terrain, RenderingSystemOpenGL* rs);
      std::shared_ptr<Terrain> _terrain;
      std::shared_ptr<Transform> _transform;

      std::shared_ptr<GLVertexArray> _terrainVao;
      std::shared_ptr<GLBuffer> _terrainVbo;
      std::map<int, std::map<int, std::shared_ptr<GLBuffer>>> _terrainIbo;
      std::map<int, std::map<int, unsigned>> _terrainNumIndices;
      void createIndexBuffer(int lod, int dir, const std::vector<unsigned>& indices);
      std::array<std::shared_ptr<GLFramebuffer>, 2> _impostorFb;
      void renderImpostor(const std::shared_ptr<Model>& tree_model, const std::shared_ptr<Model>& leaf_model, const glm::mat4& transform, RenderingSystemOpenGL* rs);

      Mat4f getWaterModelMatrix();

      std::shared_ptr<GLBuffer> _meshVbo;
      std::shared_ptr<GLBuffer> _meshIbo;

      GLint _trunkLod1BaseVertex;
      GLvoid* _trunkLod1IdxOffset;

      GLint _leafsBaseVert;
      GLvoid* _leafsIdxOffs;

      std::map<Terrain::TreeNode*, std::shared_ptr<GLVertexArray>> _treeVao;
      std::map<Terrain::TreeNode*, std::shared_ptr<GLBuffer>> _treeTransformVbo;

      std::map<Terrain::TreeNode*, std::shared_ptr<GLVertexArray>> _cloudBillboardsVao;
      std::map<Terrain::TreeNode*, std::shared_ptr<GLBuffer>> _cloudBillboardsVbo;

      std::shared_ptr<GLTexture> _heightMap;
      std::shared_ptr<GLTexture> _normalMap;
      std::shared_ptr<GLTexture> _splatMap;

      std::vector<Terrain::TreeNode*> _visibleNodes;
    };
    
    class GrassQuadTree
    {
    public:
      GrassQuadTree(int size = 32, int min_size = 2, float error_threshold = 0.7f);
     ~GrassQuadTree();
      class Node 
      {
      public:
        Node(const glm::ivec2& pos, int size);
        glm::ivec2 _pos;
        int _size;
        glm::vec2 center();
      };

      std::vector<Node*>& getNodes();
      int getSize();

    private:
      int _size;
      int _minSize;
      float _errorThreshold;
      std::vector<Node*> _nodes;
      void build(Node* node);
    };

    class BloomStage
    {
    public:
      BloomStage(const glm::ivec2& view_port_size, unsigned int level);
      ~BloomStage();

      unsigned int _level;

      void upsample();

      std::array<std::shared_ptr<GLFramebuffer>, 2> _fb;
      std::vector<std::shared_ptr<GLFramebuffer>> _upsampleFb;
      std::shared_ptr<GLTexture> _result;
    };

    class GodRayEffect
    {
    public:
      GodRayEffect(const glm::ivec2& view_port_size);
      ~GodRayEffect();
      bool render(const glm::vec3& light_pos_screen, const glm::vec3& light_pos_view_space, RenderingSystemOpenGL* rs);
      std::array<std::shared_ptr<GLFramebuffer>, 2> _fb;
      std::shared_ptr<GLFramebuffer> _resultFb;
    };

    bool _renderGodRays;

    std::vector<BloomStage> _bloomStages;
    std::shared_ptr<GodRayEffect> _godRayEffect;

    std::shared_ptr<GrassQuadTree> _grassQuadTree;

    std::map<Entity*, std::shared_ptr<TerrainRenderable> > _terrainRenderables;

    std::shared_ptr<GLTexture> _skyboxTexture;
    std::shared_ptr<GLVertexArray> _skyboxVertexArray;
    std::shared_ptr<GLBuffer> _skyboxVertexbuffer;
    std::shared_ptr<GLBuffer> _skyboxIndexbuffer;
    std::vector<unsigned int> _skyBoxIndices;

    std::shared_ptr<Mesh> _skydomeMesh;

    std::array<std::shared_ptr<GLFramebuffer>, 2> _exposureBuffer;

    std::shared_ptr<Camera> _activeCamera;

    glm::mat4 _VP;
    glm::mat4 _VPBefore;

    float _time;
    unsigned int _impostorInterval = 1000;
    float _impostorAlpha = 0.f;
    float _fpsFactor = 1.f;

    float _popupStart;
    float _popupEnd;
    bool _popupAnimate = false;

    float _aspectRatio = 1.f;

    std::vector<float> _gaussKernel = { 0.016216f, 0.054054f, 0.1216216f, 0.1945946f, 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
    std::vector<float> _sobelKernelGradient = { 1.f, 0.f, -1.f };
    std::vector<float> _sobelKernelSmooth = { 1.f, 2.f, 1.f };

    void prepareRender(float delta_time, float time);
    void renderShadowMaps();
    void postProcessing(float delta_time);
    void swapParams();
    void renderToScreen();
    void geometryPass();
    void renderModels();
    void renderTerrain(TerrainRenderMode mode);
    void renderLightSources();
    void lightingDirectional(Entity* directional_light);
    void lightingSpot(Entity* spot_light);
    void lightingPoint(Entity* point_light);
    void lightingPass();
    void godRayPostProcess();
    void volumeLightPostProcess();
    void renderLightVolumeDirectional(Entity* light);
    bool renderLightVolumeSpot(Entity* light);
    void renderLightVolumePoint(Entity* light);
    void bloomPostProcess();
    void eyeAdaptionPostProcess();
    void depthOfFieldPostProcess();
    void computeGradient();
    void pingPongFilter(unsigned int steps, const std::array<std::shared_ptr<GLFramebuffer>, 2>& fb,
      const std::vector<float>& kernel_horizontal, const std::vector<float>& kernel_vertical, const std::shared_ptr<GLTexture>& weight_texture = nullptr, const std::shared_ptr<GLTexture>& base_texture = nullptr);
    void renderSkybox();
    void renderSkydome();
    void renderShadowMapsDirectional(Entity* entity);
    void renderShadowMapsSpot(Entity* entity);
    void renderShadowMapsPoint(Entity* entity);
    void renderMeshGeometryDeferred(Entity* entity);
    bool isCulled(const std::array<glm::vec3, 8>& bb_modelspace, const glm::mat4& mvp);
    bool isCulled(const std::shared_ptr<Mesh>& mesh, const glm::mat4& model_matrix);
    bool renderGodRaysScreenSpace(const glm::vec3& light_pos);
    void renderAABBs();
    void renderParticles();
    void renderWireFrame();
    void ssrPostProcess();
    void renderTrees(Entity* directional_light);
    void renderImpostors();
    void createShader(std::shared_ptr<GLShaderProgram>& shader, const std::string& vs, const std::string& fs, const std::string& gs = "");
    void renderWater();
    void renderDirectionalLights();
    void initFramebuffers();

    struct AsyncTextureResult
    {
      unsigned char* _data;
      int _width;
      int _height;
      int _channels;
    };
    std::map<std::string, std::shared_future<AsyncTextureResult>> _textureFutures;
    void processTextureFutures();
    void bindTextureOrLoadAsync(const std::string& path);

    void setupMeshBindings(const std::shared_ptr<Mesh>& mesh);

    inline glm::vec3 toScreenSpace(glm::mat4 model_matrix, const glm::vec3& pos_model_space, const glm::mat4& view_matrix, const glm::mat4& projection_matrix)
    {
      auto pos_screen_hat = projection_matrix * view_matrix * model_matrix * glm::vec4(pos_model_space, 1.f);
      glm::vec3 pos_screen_ndc(pos_screen_hat.x / pos_screen_hat.w,
        pos_screen_hat.y / pos_screen_hat.w, pos_screen_hat.z / pos_screen_hat.w);
      return (1.f + pos_screen_ndc) * 0.5f;
    }

  };
}

#endif
