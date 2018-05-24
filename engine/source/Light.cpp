#include "Light.h"
#include "RenderingSystem.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <math/MathHelpers.h>

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

  Mat4f DirectionalLight::getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov_degrees, const Mat4f& view_matrix_inverse,
    const Mat4f& view_matrix_light, float shadow_map_size, const std::vector<float>& frustum_splits, StackPOD<Mat4f>& vp, ZNearMapping z_near_mapping)
  {
    vp.clear();
    Vec3f global_min(std::numeric_limits<float>::max());
    Vec3f global_max(std::numeric_limits<float>::lowest());
    for (unsigned int i = 0; i < frustum_splits.size(); i++) {
      auto projection_matrix = MathHelpers::getProjectionMatrixPerspective(fov_degrees, aspect_ratio, i == 0 ? near_plane : frustum_splits[i - 1], frustum_splits[i], z_near_mapping);
      Mat4f p_inverse = inverse(projection_matrix);
      auto vp_inverse_v_light = view_matrix_light * view_matrix_inverse * p_inverse;
      auto cube_ndc = MathHelpers::cubeNDC(z_near_mapping);
      auto computeAABB = [&cube_ndc] (const Mat4f& transform) {
        Vec3f bb_min(std::numeric_limits<float>::max());
        Vec3f bb_max(std::numeric_limits<float>::lowest());
        for (const auto& v : cube_ndc) {
          auto corner_h = transform * Vec4f(v, 1.f);
          auto corner = corner_h.xyz() / corner_h[3]; // perspective division
          corner[2] = -corner[2]; // invert z
          bb_min = minimum(bb_min, corner);
          bb_max = maximum(bb_max, corner);
        }
        return AABB(bb_min, bb_max);
      };
      auto aabb_view = computeAABB(p_inverse);
      auto aabb_light = computeAABB(vp_inverse_v_light);
      float sphere_radius = std::numeric_limits<float>::lowest();
      for (unsigned char i = 0; i < 8; i++) {
        sphere_radius = (std::max)(sphere_radius, distance(aabb_view.center(), aabb_view.getVertex(i)));
      }
      Vec3f bb_min = aabb_light.center() - sphere_radius;
      Vec3f bb_max = aabb_light.center() + sphere_radius;

      // Avoids shimmering edges when the camera is moving
      Vec2f units_per_texel = (bb_max.xy() - bb_min.xy()) / shadow_map_size;
      Vec2f bb_min_xy = floor(bb_min.xy() / units_per_texel) * units_per_texel;
      Vec2f bb_max_xy = ceil(bb_max.xy() / units_per_texel) * units_per_texel;
      bb_min = Vec3f(bb_min_xy, bb_min[2]);
      bb_max = Vec3f(bb_max_xy, bb_max[2]);

      vp.push_back_secure(MathHelpers::getProjectionMatrixOrtho(bb_min, bb_max, z_near_mapping) * view_matrix_light);

      global_min = minimum(global_min, bb_min);
      global_max = maximum(global_max, bb_max);
    }
    return MathHelpers::getProjectionMatrixOrtho(global_min, global_max, z_near_mapping) * view_matrix_light;
  }

  Mat4f DirectionalLight::getViewMatrix()
  {
    auto direction = normalize(_target - _pos);
    auto up = normalize(Vec3f(-direction[1], direction[0], 0.f));
    auto right = Vec3f(cross(glm::vec3(direction), glm::vec3(up)));
    return MathHelpers::getViewMatrixRightHanded(_pos, right, up, direction);
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