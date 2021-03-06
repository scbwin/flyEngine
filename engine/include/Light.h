#ifndef LIGHT_H
#define LIGHT_H

#include "math/FlyMath.h"
#include "Mesh.h"
#include <memory>
#include <ZNearMapping.h>
#include <StackPOD.h>

namespace fly
{
  class RenderingSystem;
  class AbstractWindow;

  class Light
  {
  public:
    Light(const Vec3f& color);
    const Vec3f& getIntensity() const;
    void setIntensity(const Vec3f& i);
    float getLensflareWeight() const;
    float getLensflareRefSamplesPassed() const;
  protected:
    float _lensFlareWeight = 1.f;
    float _lensFlareRefSamplesPassed = 100.f;
    Vec3f _intensity = Vec3f(1.f);
  //  Vec3f _target;
  //  Vec3f _pos;
  };

  class DirectionalLight : public Light
  {
  public:
    DirectionalLight(const Vec3f& color, const Vec3f& direction);
    float _ambientPower = 0.3f;
    void getViewProjectionMatrices(float aspect_ratio, float near_plane, float fov_degrees, const Mat4f& view_matrix_inverse,
      float shadow_map_size, const std::vector<float>& frustum_splits, ZNearMapping z_near_mapping, StackPOD<Mat4f>& vp, StackPOD<Mat4f>& vp_light_volume);
    Mat4f getViewMatrix(const Vec3f& pos) const;
    const Vec3f& getDirection() const;
    void setDirection(const Vec3f& direction);
    void setMaxShadowCastDistance(float distance);
    float getMaxShadowCastDistance() const;
  private:
    Vec3f _direction;
    float _maxShadowCastDistance = 1000.f;
  };

  class SpotLight : public Light
  {
  public:
    SpotLight(const Vec3f& color, const Vec3f& pos, const Vec3f& target, float near, float far, float penumbra_degrees, float umbra_degrees);

    float _zNear;
    float _zFar;
    float _penumbraDegrees;
    float _umbraDegrees;
    Vec3f _target;
    Vec3f _pos;

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
    PointLight(const Vec3f& color, float near, float far);
    void getViewProjectionMatrices(std::vector<glm::mat4>& vp);

    float _zNear;
    float _zFar;
    Vec3f _target;
    Vec3f _pos;

  };
}

#endif