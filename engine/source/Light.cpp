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
  Light::Light(const Vec3f & intensity, const Vec3f& pos, const Vec3f& target) : _intensity(intensity), _pos(pos), _target(target)
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

  const Vec3f & Light::getTarget() const
  {
    return _target;
  }

  void Light::setTarget(const Vec3f & target)
  {
    _target = target;
  }

  const Vec3f & Light::getPosition() const
  {
    return _pos;
  }

  void Light::setPosition(const Vec3f & position)
  {
    _pos = position;
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
    Light(color, pos, target), _zNear(near), _zFar(far), _penumbraDegrees(penumbra_degrees), _umbraDegrees(umbra_degrees)
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

  DirectionalLight::DirectionalLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target) :
    Light(color, pos, target)
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
    auto view_matrix_light = getViewMatrix();
    for (unsigned int i = 0; i < frustum_splits.size(); i++) {
      auto projection_matrix = MathHelpers::getProjectionMatrixPerspective(fov_degrees, aspect_ratio, i == 0 ? near_plane : frustum_splits[i - 1], frustum_splits[i], z_near_mapping);
      auto cube_ndc = MathHelpers::cubeNDC(z_near_mapping);
      Vec3f corners_light[cube_ndc.size()];
      for (size_t i = 0; i < cube_ndc.size(); i++) {
        auto corner_light_h = view_matrix_light * view_matrix_inverse * inverse(projection_matrix) * Vec4f(cube_ndc[i], 1.f);
        corners_light[i] = corner_light_h.xyz() / corner_light_h[3];
      }
      AABB aabb_light(corners_light, cube_ndc.size());

      // Prevents shimmering edges when the camera is moving
     auto units_per_texel = (aabb_light.getMax() - aabb_light.getMin()) / shadow_map_size;
     aabb_light.getMin() = floor(aabb_light.getMin() / units_per_texel) * units_per_texel;
     aabb_light.getMax() = floor(aabb_light.getMax() / units_per_texel) * units_per_texel;

      vp.push_back_secure(MathHelpers::getProjectionMatrixOrtho(aabb_light.getMin(), aabb_light.getMax(), z_near_mapping) * view_matrix_light); // Used for rendering
      vp_light_volume.push_back_secure(MathHelpers::getProjectionMatrixOrtho(Vec3f(aabb_light.getMin().xy(), 0.f), aabb_light.getMax(), z_near_mapping) * view_matrix_light); // Used for culling
    }
  }

  Mat4f DirectionalLight::getViewMatrix()
  {
    auto direction = normalize(_target - _pos);
    auto up = normalize(Vec3f(-direction[1], direction[0], 0.f));
    auto right = Vec3f(cross(glm::vec3(direction), glm::vec3(up)));
    return MathHelpers::getViewMatrixLeftHanded(_pos, right, up, direction);
  }

  PointLight::PointLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, float near, float far) : Light(color, pos, target), _zNear(near), _zFar(far)
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