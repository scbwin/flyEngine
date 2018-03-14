#ifndef LIGHT_H
#define LIGHT_H

#include "math/FlyMath.h"
#include "Mesh.h"
#include <memory>
#include "Component.h"

namespace fly
{
  class RenderingSystem;
  class AbstractWindow;

  class Light : public Component
  {
  public:
    Light(const glm::vec3& color);
    glm::vec3 _color;
    glm::vec3 _target;
    float _lensFlareWeight = 1.f;
    float _lensFlareRefSamplesPassed = 100.f;
  };

  class DirectionalLight : public Light
  {
  public:
    DirectionalLight(const glm::vec3& color, const std::vector<float>& csm_distances);
    std::vector<float> _csmDistances;
    float _ambientPower = 0.3f;
    void getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov, const Mat4f& view_matrix, 
      const Mat4f& view_matrix_light, float shadow_map_size, std::vector<Mat4f>& vp, bool directx = false);
    glm::mat4 getViewMatrix(const glm::vec3& pos);
  };

  class SpotLight : public Light
  {
  public:
    SpotLight(const glm::vec3& color, float near, float far, float penumbra_degrees, float umbra_degrees);

    float _zNear;
    float _zFar;
    float _penumbraDegrees;
    float _umbraDegrees;

    template<class T>
    T smoothstep(T edge0, T edge1, T x) const
    {
      T t = glm::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
      return t * t * (3.f - 2.f * t);
    }

    void getViewProjectionMatrix(glm::mat4& view_matrix, glm::mat4& projection_matrix, const glm::vec3& pos_world);
  };

  class PointLight : public Light
  {
  public:
    PointLight(const glm::vec3& color, float near, float far);
    void getViewProjectionMatrices(const glm::vec3& pos_world_space, std::vector<glm::mat4>& vp);

    float _zNear;
    float _zFar;

  };
}

#endif