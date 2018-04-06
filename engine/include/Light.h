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
    Light(const Vec3f& color, const Vec3f& pos, const Vec3f& target);
    Vec3f _color;
    Vec3f _target;
    Vec3f _pos;
    float _lensFlareWeight = 1.f;
    float _lensFlareRefSamplesPassed = 100.f;
    const Vec3f& getIntensity() const;
    void setIntensity(const Vec3f& i);
  private:
    Vec3f _intensity = Vec3f(1.f);
  };

  class DirectionalLight : public Light
  {
  public:
    DirectionalLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, const std::vector<float>& csm_distances);
    std::vector<float> _csmDistances;
    float _ambientPower = 0.3f;
    void getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov, const Mat4f& view_matrix, 
      const Mat4f& view_matrix_light, float shadow_map_size, std::vector<Mat4f>& vp, bool directx = false);
    glm::mat4 getViewMatrix();
  };

  class SpotLight : public Light
  {
  public:
    SpotLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, float near, float far, float penumbra_degrees, float umbra_degrees);

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

    void getViewProjectionMatrix(glm::mat4& view_matrix, glm::mat4& projection_matrix);
  };

  class PointLight : public Light
  {
  public:
    PointLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, float near, float far);
    void getViewProjectionMatrices(std::vector<glm::mat4>& vp);

    float _zNear;
    float _zFar;

  };
}

#endif