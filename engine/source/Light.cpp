#include "Light.h"
#include "RenderingSystem.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <math/MathHelpers.h>
#include <Sphere.h>

namespace fly
{
  Light::Light(const Vec3f & intensity) : _intensity(intensity)
  {
  }

  const Vec3f& Light::getIntensity() const
  {
    return _intensity;
  }

  void Light::setIntensity(const Vec3f& i)
  {
    _intensity = i;
  }

  float Light::getLensflareWeight() const
  {
    return _lensFlareWeight;
  }

  float Light::getLensflareRefSamplesPassed() const
  {
    return _lensFlareRefSamplesPassed;
  }

  SpotLight::SpotLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, float near, float far, float penumbra_degrees, float umbra_degrees) :
    Light(color), _pos(pos), _target(target), _zNear(near), _zFar(far), _penumbraDegrees(penumbra_degrees), _umbraDegrees(umbra_degrees)
  {
    _lensFlareWeight = 0.11f;
  }

  void SpotLight::getViewProjectionMatrix(glm::mat4& view_matrix, glm::mat4& projection_matrix)
  {
    projection_matrix = glm::perspective(glm::radians(_umbraDegrees * 2.f), 1.f, _zNear, _zFar);
    glm::vec3 direction = normalize(_target - _pos);
    glm::vec3 up = cross(direction, normalize(direction + glm::vec3(1.f, 0.f, 0.f))); // arbitrary vector that is orthogonal to direction
    view_matrix = glm::lookAt(glm::vec3(_pos), glm::vec3(_target), up);
  }

  DirectionalLight::DirectionalLight(const Vec3f& color, const Vec3f& direction) :
    Light(color),
    _direction(normalize(direction))
  {
    _lensFlareWeight = 2.f;
    _lensFlareRefSamplesPassed = 4000.f;
  }

  void DirectionalLight::getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov_degrees, 
    const Mat4f& view_matrix_inverse, float shadow_map_size, const std::vector<float>& frustum_splits, 
    ZNearMapping z_near_mapping, StackPOD<Mat4f>& vp, StackPOD<Mat4f>& vp_light_volume)
  {
    vp.clear();
    vp_light_volume.clear();
    for (unsigned int i = 0; i < frustum_splits.size(); i++) {
      auto projection_matrix = MathHelpers::getProjectionMatrixPerspective(fov_degrees, aspect_ratio, i == 0 ? near_plane : frustum_splits[i - 1], frustum_splits[i], z_near_mapping);
      auto cube_ndc = MathHelpers::cubeNDC(z_near_mapping);
      Vec3f center_world(0.f);
      for (const auto& c : cube_ndc) {
        auto pos_world_h = view_matrix_inverse * inverse(projection_matrix) * Vec4f(c, 1.f);
        center_world += (pos_world_h.xyz() / pos_world_h[3]) / 8.f;
      }
      auto view_matrix_light = getViewMatrix(center_world);
      AABB aabb_light(AABB::fromTransform<cube_ndc.size()>(cube_ndc.data(), view_matrix_light * view_matrix_inverse * inverse(projection_matrix)));

      // Prevents shimmering edges when the camera is moving
     auto units_per_texel = (aabb_light.getMax() - aabb_light.getMin()) / shadow_map_size;
     aabb_light.getMin() = floor(aabb_light.getMin() / units_per_texel) * units_per_texel;
     aabb_light.getMax() = floor(aabb_light.getMax() / units_per_texel) * units_per_texel;

      vp.push_back_secure(MathHelpers::getProjectionMatrixOrtho(aabb_light.getMin() , aabb_light.getMax(), z_near_mapping) * view_matrix_light); // Used for rendering
      vp_light_volume.push_back_secure(MathHelpers::getProjectionMatrixOrtho(Vec3f(aabb_light.getMin().xy(), -_maxShadowCastDistance), aabb_light.getMax(), z_near_mapping) * view_matrix_light); // Used for culling
    }
  }

  Mat4f DirectionalLight::getViewMatrix(const Vec3f& pos) const
  {
    auto up = normalize(Vec3f(-_direction[1], _direction[0], 0.f));
    auto right = Vec3f(cross(glm::vec3(_direction), glm::vec3(up)));
    return MathHelpers::getViewMatrixLeftHanded(pos, right, up, _direction);
  }

  const Vec3f & DirectionalLight::getDirection() const
  {
    return _direction;
  }

  void DirectionalLight::setDirection(const Vec3f & direction)
  {
    _direction = normalize(direction);
  }

  void DirectionalLight::setMaxShadowCastDistance(float distance)
  {
    _maxShadowCastDistance = distance;
  }

  float DirectionalLight::getMaxShadowCastDistance() const
  {
    return _maxShadowCastDistance;
  }

  PointLight::PointLight(const Vec3f& color, float near, float far) : Light(color), _zNear(near), _zFar(far)
  {
    _lensFlareWeight = 0.15f;
  }
  void PointLight::getViewProjectionMatrices(std::vector<glm::mat4>& vp)
  {
    auto projection_matrix = glm::perspective(glm::radians(90.f), 1.f, _zNear, _zFar);

    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)));
    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)));
    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)));
    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)));
    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, -1.f, 0.f)));
    vp.push_back(projection_matrix * glm::lookAt(glm::vec3(_pos), glm::vec3(_pos) + glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, -1.f, 0.f)));
  }
}