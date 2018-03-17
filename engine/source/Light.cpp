#include "Light.h"
#include "RenderingSystem.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace fly
{
  Light::Light(const Vec3f & color, const Vec3f& pos, const Vec3f& target) : _color(color), _pos(pos), _target(target)
  {
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

  DirectionalLight::DirectionalLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, const std::vector<float>& csm_distances) :
    Light(color, pos, target), _csmDistances(csm_distances)
  {
    _lensFlareWeight = 2.f;
    _lensFlareRefSamplesPassed = 4000.f;
  }

  void DirectionalLight::getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov, const Mat4f& view_matrix_inverse,
    const Mat4f& view_matrix_light, float shadow_map_size, std::vector<Mat4f>& vp, bool directx)
  {
    assert(!vp.size());
    for (unsigned int i = 0; i < _csmDistances.size(); i++) {
      float near = i == 0 ? near_plane : _csmDistances[i - 1];
      glm::mat4 projection_matrix = directx ? glm::perspectiveZO(glm::radians(fov), aspect_ratio, near, _csmDistances[i]) 
        : glm::perspective(glm::radians(fov), aspect_ratio, near, _csmDistances[i]);
      Mat4f p_inverse = inverse(projection_matrix);
      auto vp_inverse_v_light = view_matrix_light * view_matrix_inverse * p_inverse;

      std::array<Vec3f, 8> cube_ndc = { Vec3f({-1.f, -1.f, directx ? 0.f : -1.f}), Vec3f({1.f, -1.f, directx ? 0.f : -1.f}), Vec3f({-1.f, 1.f, directx ? 0.f : -1.f}), Vec3f({-1.f, -1.f, 1.f}),
        Vec3f({1.f, 1.f, directx ? 0.f : -1.f}), Vec3f({1.f, -1.f, 1.f}), Vec3f({-1.f, 1.f, 1.f}), Vec3f({1.f, 1.f, 1.f}) };

      std::vector<Vec3f> cam_frustum_light_space, cam_frustum_view_space;
      Vec3f light_space_frustum_center(0.f);
      Vec3f view_space_frustum_center(0.f);
      for (unsigned int j = 0; j < cube_ndc.size(); j++) {
        auto corner_light = vp_inverse_v_light * Vec4f({ cube_ndc[j][0], cube_ndc[j][1], cube_ndc[j][2], 1.f });
        cam_frustum_light_space.push_back(Vec3f({ corner_light[0] / corner_light[3], corner_light[1] / corner_light[3], corner_light[2] / corner_light[3] }));
        cam_frustum_light_space[j][2] = -cam_frustum_light_space[j][2];
        light_space_frustum_center += cam_frustum_light_space[j];
        auto corner_view_space = p_inverse * Vec4f({ cube_ndc[j][0], cube_ndc[j][1], cube_ndc[j][2], 1.f });
        cam_frustum_view_space.push_back(Vec3f({ corner_view_space[0] / corner_view_space[3], corner_view_space[1] / corner_view_space[3], corner_view_space[2] / corner_view_space[3] }));
        cam_frustum_view_space[j][2] = -cam_frustum_view_space[j][2];
        view_space_frustum_center += cam_frustum_view_space[j];
      }
      light_space_frustum_center /= static_cast<float>(cam_frustum_light_space.size());
      view_space_frustum_center /= static_cast<float>(cam_frustum_view_space.size());

      // Computing the bounding sphere radius in view space is numerically more stable, because P^-1 doesn't change when the camera is moving.
      float sphere_radius_view_space = std::numeric_limits<float>::lowest();
      for (unsigned int j = 0; j < 8; j++) {
        sphere_radius_view_space = (std::max)(sphere_radius_view_space, distance(view_space_frustum_center, cam_frustum_view_space[j]));
      }
      Vec3f bb_min (light_space_frustum_center - sphere_radius_view_space);
      Vec3f bb_max(light_space_frustum_center + sphere_radius_view_space);

      // Avoids shimmering edges when the camera is moving
      Vec3f units_per_texel = (bb_max - bb_min) / shadow_map_size;
      bb_min = floor(bb_min / units_per_texel) * units_per_texel;
      bb_max = ceil(bb_max / units_per_texel) * units_per_texel;

      vp.push_back(Mat4f(directx ? glm::orthoZO(bb_min[0], bb_max[0], bb_min[1], bb_max[1], bb_min[2], bb_max[2]) : glm::ortho(bb_min[0], bb_max[0], bb_min[1], bb_max[1], bb_min[2], bb_max[2])) * view_matrix_light);
    }
  }

  glm::mat4 DirectionalLight::getViewMatrix()
  {
    auto dir = normalize(_target - _pos);
    return glm::lookAt(glm::vec3(_pos), glm::vec3(_target), normalize(glm::vec3(-dir[1], dir[0], 0.f)));
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