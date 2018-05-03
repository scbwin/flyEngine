#ifndef RENDERINTERFACE_H
#define RENDERINTERFACE_H

#include <memory>
#include <set>
#include "System.h"
#include <glm/glm.hpp>
#include <array>
#include <math/FlyMath.h>

namespace fly
{
  class Entity;
  class Mesh;
  class Terrain;

  class RenderingSystem : public System
  {
  public:
    virtual void init(const Vec2i& window_size) = 0;
    virtual void setSkybox(const std::array<std::string, 6u>& paths) = 0;
    virtual void setSkydome(const std::shared_ptr<Mesh>& mesh) = 0;
    virtual ~RenderingSystem();
    virtual void initShaders() = 0;
    virtual void onComponentsChanged(Entity* entity) override;
    virtual void onDirectionalLightAdded(Entity* entity, bool always_create) = 0;
    virtual void onSpotLightAdded(Entity* entity, bool always_create) = 0;
    virtual void onPointLightAdded(Entity* entity, bool always_create) = 0;
    virtual void onLightRemoved(Entity* entity) = 0;
    virtual void onTerrainAdded(Entity* entity) = 0;
    virtual void onTerrainRemoved(Entity* entity) = 0;
    virtual void onResize(const glm::ivec2& size);
    virtual float getSceneDepth(const glm::ivec2& pos) = 0;
    virtual glm::mat4 getViewMatrix();
    virtual glm::mat4 getProjectionMatrix();
    virtual void setBackgroundColor(const glm::vec4& color);
    void getRayCast(float x, float y, float viewPortWidth, float viewPortHeight, const glm::mat4& view_matrix, glm::vec3& pos, glm::vec3& d);
    glm::mat4 getProjectionMatrix(int width, int height);

    Mat4f _viewMatrix;
    Mat4f _projectionMatrix;
    float _zNear = 1.f;
    float _zFar = 10000.f;
    float _fovDegrees = 60.f;
    float _reflectanceStepsize = 0.1f;
    int _raytracingStepsReflections = 32;
    unsigned int _godRaySamples = 48;
    float _godRayDecay = 0.9f;

    float _exposure = 1.f;
    float _lerpAlpha = 0.95f;
    float _motionBlurStrength = 1.3f;
    glm::vec4 _backGroundColor = glm::vec4(0.427f, 0.650f, 0.909f, 1.0f);
    bool _ssrEnabled = false;
    bool _parallaxOcclusionEnabled = true;
    bool _ssaoEnabled = false;
    bool _smoothParticles = false;
    bool _particleShadows = false;
    bool _gammaCorrectionEnabled = true;
    bool _underwaterDistortionEnabled = true;
    bool _waterPopupDistortionEnabled = true;
    bool _smoothCamera = true;
    float _godRayWeight = 3.f;
    float _ssaoRadius = 0.5f;
    unsigned int _bloomBlurSteps = 1;
    unsigned int _bloomDebugIdx = 0;
    std::array<float, 3> _bloomWeights = { 1.f, 1.f, 1.f };
    bool _eyeAdaptionEnabled = true;
    bool _lensFlaresEnabled = true;

    glm::ivec2 _viewportSize = glm::ivec2(0);

    bool isBloomEnabled();
    void setBloomEnabled(bool enabled);
    bool isGodRaysEnabled();
    void setGodRaysEnabled(bool enabled);
    bool isLightVolumesEnabled();
    void setLightVolumesEnabled(bool enabled);
    bool isMotionBlurEnabled();
    void setMotionBlurEnabled(bool enabled);
    bool isShadowsEnabled();
    void setShadowsEnabled(bool enabled);
    bool isNormalMappingEnabled();
    void setNormalMappingEnabled(bool enabled);
    std::array<float, 3> getBloomWeights();
    void setBloomWeight(unsigned int idx, float weight);
    unsigned int getVolumeLightSamples();
    void setVolumeLightSamples(unsigned int samples);
    float getVolumeLightWeight();
    void setVolumeLightWeight(float weight);
    unsigned int getShadowPCFSamples();
    void setShadowPCFSamples(unsigned int samples);
    bool isDepthOfFieldEnabled();
    void setDepthOfFieldEnabled(bool enabled);
    bool isFrustumCullingEnabled();
    void setFrustumCullingEnabled(bool enabled);
    bool getRenderAABBs();
    void setRenderAABBs(bool render);
    void setDofParam(const glm::vec3& param);
    glm::vec3 getDofParam() const;
    void setRenderWireFrame(bool render_wireframe);
    bool getRenderWireFrame() const;

    enum class RenderStage
    {
      MODELS,
      TERRAIN,
      GRASS,
      TREES,
      TREES_SHADOW_MAP,
      LIGHTING,
      BLOOM,
      SHADOWMAP
    };

  protected:
    std::set<Entity*> _models;
    std::set<Entity*> _directionalLights;
    std::set<Entity*> _spotLights;
    std::set<Entity*> _pointLights;
    std::set<Entity*> _lights;
    std::set<Entity*> _cameras;
    std::set<Entity*> _billboards;

    bool _bloomEnabled = true;
    bool _godRaysEnabled = true;
    bool _lightVolumesEnabled = true;
    bool _motionBlurEnabled = true;
    bool _shadowsEnabled = true;
    bool _normalMappingEnabled = true;

    unsigned int _volumeLightSamples = 128;
    float _volumeLightWeight = 0.35f;
    unsigned int _shadowPCFSamples = 4;
    bool _lightVolumeBilateralBlur = true;

    bool _depthOfFieldEnabled = true;
    bool _frustumCullingEnabled = true;

    bool _renderWireFrame = false;

    glm::vec3 _dofParam = glm::vec3(0.4f, 3.f, 5000.f); // near, focus, far

    glm::vec3 _camPosBefore = glm::vec3(0.f);
    glm::vec3 _eulerAnglesBefore;

    bool _renderAABBs = false;
  };
}

#endif
