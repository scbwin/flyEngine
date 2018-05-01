#include "RenderingSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "Entity.h"
#include <Camera.h>
#include <Model.h>
#include <Transform.h>
#include <Light.h>
#include <Billboard.h>
#include <Terrain.h>

namespace fly
{
  RenderingSystem::~RenderingSystem()
  {}

  void RenderingSystem::onComponentsChanged(Entity* entity)
  {
    auto model = entity->getComponent<Model>();
    auto transform = entity->getComponent<Transform>();
    auto directional_light = entity->getComponent<DirectionalLight>();
    auto spot_light = entity->getComponent<SpotLight>();
    auto point_light = entity->getComponent<PointLight>();
    auto camera = entity->getComponent<Camera>();
    auto billboard = entity->getComponent<Billboard>();
    auto geo_mip_map = entity->getComponent<Terrain>();

    bool is_light = directional_light || spot_light || point_light;

    if (model != nullptr && transform != nullptr) {
      _models.insert(entity);
    }
    else {
      _models.erase(entity);
    }

    if (geo_mip_map && transform) {
      onTerrainAdded(entity);
    }
    else {
      onTerrainRemoved(entity);
    }

    if (directional_light != nullptr) {
      _directionalLights.insert(entity);
      onDirectionalLightAdded(entity, false);
    }
    else {
      _directionalLights.erase(entity);
    }

    if (spot_light != nullptr) {
      _spotLights.insert(entity);
      onSpotLightAdded(entity, false);
    }
    else {
      _spotLights.erase(entity);
    }

    if (point_light != nullptr) {
      _pointLights.insert(entity);
      onPointLightAdded(entity, false);
    }
    else {
      _pointLights.erase(entity);
    }

    if (is_light) {
      _lights.insert(entity);
    }
    else {
      _lights.erase(entity);
      onLightRemoved(entity);
    }

    if (camera != nullptr) {
      _cameras.insert(entity);
      _camPosBefore = camera->getPosition();
      _eulerAnglesBefore = camera->getEulerAngles();
    }
    else {
      _cameras.erase(entity);
    }
    
    if (billboard && transform) {
      _billboards.insert(entity);
    }
    else {
      _billboards.erase(entity);
    }
  }

  void RenderingSystem::onResize(const glm::ivec2& size)
  {
    _viewportSize = size;
  }

  glm::mat4 RenderingSystem::getViewMatrix()
  {
    return _viewMatrix;
  }

  glm::mat4 RenderingSystem::getProjectionMatrix()
  {
    return _projectionMatrix;
  }

  void RenderingSystem::setBackgroundColor(const glm::vec4 & color)
  {
    _backGroundColor = color;
  }

  void RenderingSystem::getRayCast(float pixel_x, float pixel_y, float viewPortWidth, float viewPortHeight, const glm::mat4 & view_matrix, glm::vec3& pos, glm::vec3& d)
  {
   /* auto vp_inverse = glm::inverse(getProjectionMatrix(viewPortWidth, viewPortHeight) * view_matrix);
    
    glm::vec4 p_near_hat(2.f * x / viewPortWidth - 1.f, 2.f * y / viewPortHeight - 1.f, -1.f, 1.f);
    glm::vec4 p_far_hat(2.f * x / viewPortWidth - 1.f, 2.f * y / viewPortHeight - 1.f, 1.f, 1.f);
    auto p_near_star = vp_inverse * p_near_hat;
    auto p_far_star = vp_inverse * p_far_hat;
    glm::vec3 p_near(p_near_star.x / p_near_star.w, p_near_star.y / p_near_star.w, p_near_star.z / p_near_star.w);
    glm::vec3 p_far(p_far_star.x / p_far_star.w, p_far_star.y / p_far_star.w, p_far_star.z / p_far_star.w);*/

    auto p_normalized = glm::vec2(pixel_x, pixel_y) / glm::vec2(viewPortWidth, viewPortHeight);
    auto p_ndc = p_normalized * 2.f - 1.f;
    auto vp_inverse = glm::inverse(getProjectionMatrix(viewPortWidth, viewPortHeight) * view_matrix);

    auto p_near = vp_inverse * glm::vec4(p_ndc, -1.f, 1.f);
    auto p_far = vp_inverse * glm::vec4(p_ndc, 1.f, 1.f);
    
    p_near /= p_near.w;
    p_far /= p_far.w;

    d = glm::normalize(p_far - p_near);
    pos = p_near;
  }

  glm::mat4 RenderingSystem::getProjectionMatrix(int width, int height)
  {
    if (width == 0 || height == 0) {
      return glm::mat4();
    }
    return glm::perspective(glm::radians(_fov), static_cast<float>(width) / static_cast<float>(height), _zNear, _zFar);
  }
  bool RenderingSystem::isBloomEnabled()
  {
    return _bloomEnabled;
  }
  void RenderingSystem::setBloomEnabled(bool enabled)
  {
    _bloomEnabled = enabled;
  }
  bool RenderingSystem::isGodRaysEnabled()
  {
    return _godRaysEnabled;
  }
  void RenderingSystem::setGodRaysEnabled(bool enabled)
  {
    _godRaysEnabled = enabled;
  }
  bool RenderingSystem::isLightVolumesEnabled()
  {
    return _lightVolumesEnabled;
  }
  void RenderingSystem::setLightVolumesEnabled(bool enabled)
  {
    _lightVolumesEnabled = enabled;
  }
  bool RenderingSystem::isMotionBlurEnabled()
  {
    return _motionBlurEnabled;
  }
  void RenderingSystem::setMotionBlurEnabled(bool enabled)
  {
    _motionBlurEnabled = enabled;
  }
  bool RenderingSystem::isShadowsEnabled()
  {
    return _shadowsEnabled;
  }
  void RenderingSystem::setShadowsEnabled(bool enabled)
  {
    _shadowsEnabled = enabled;
  }
  bool RenderingSystem::isNormalMappingEnabled()
  {
    return _normalMappingEnabled;
  }
  void RenderingSystem::setNormalMappingEnabled(bool enabled)
  {
    _normalMappingEnabled = enabled;
  }
  std::array<float, 3> RenderingSystem::getBloomWeights()
  {
    return _bloomWeights;
  }
  void RenderingSystem::setBloomWeight(unsigned int idx, float weight)
  {
    idx = std::min(idx, static_cast<unsigned int>(_bloomWeights.size()));
    _bloomWeights[idx] = weight;
  }
  unsigned int RenderingSystem::getVolumeLightSamples()
  {
    return _volumeLightSamples;
  }
  void RenderingSystem::setVolumeLightSamples(unsigned int samples)
  {
    _volumeLightSamples = samples;
  }
  float RenderingSystem::getVolumeLightWeight()
  {
    return _volumeLightWeight;
  }
  void RenderingSystem::setVolumeLightWeight(float weight)
  {
    _volumeLightWeight = weight;
  }
  unsigned int RenderingSystem::getShadowPCFSamples()
  {
    return _shadowPCFSamples;
  }
  void RenderingSystem::setShadowPCFSamples(unsigned int samples)
  {
    _shadowPCFSamples = glm::clamp(1u, samples, 16u);
  }
  bool RenderingSystem::isDepthOfFieldEnabled()
  {
    return _depthOfFieldEnabled;
  }
  void RenderingSystem::setDepthOfFieldEnabled(bool enabled)
  {
    _depthOfFieldEnabled = enabled;
  }
  bool RenderingSystem::isFrustumCullingEnabled()
  {
    return _frustumCullingEnabled;
  }
  void RenderingSystem::setFrustumCullingEnabled(bool enabled)
  {
    _frustumCullingEnabled = enabled;
  }
  bool RenderingSystem::getRenderAABBs()
  {
    return _renderAABBs;
  }
  void RenderingSystem::setRenderAABBs(bool render)
  {
    _renderAABBs = render;
  }
  void RenderingSystem::setDofParam(const glm::vec3 & param)
  {
    _dofParam = param;
  }
  glm::vec3 RenderingSystem::getDofParam() const
  {
    return _dofParam;
  }
  void RenderingSystem::setRenderWireFrame(bool render_wireframe)
  {
    _renderWireFrame = render_wireframe;
  }
  bool RenderingSystem::getRenderWireFrame() const
  {
    return _renderWireFrame;
  }
}