#ifndef MATHHELPERS_H
#define MATHHELPERS_H

#include <math/FlyMath.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <renderer/RenderParams.h>

namespace fly
{
  class MathHelpers
  {
  public:
    static inline std::array<Vec3f, 8> cubeNDC(ZNearMapping z_near_mapping)
    {
      std::array<Vec3f, 8> ret;
      float near_ndc = z_near_mapping == ZNearMapping::MINUS_ONE ? -1.f : 0.f;
      for (unsigned char i = 0; i < 8; i++) {
        ret[i] = Vec3f(i & 4 ? -1.f : 1.f, i & 2 ? -1.f : 1.f, i & 1 ? near_ndc : 1.f);
      }
      return ret;
    }
    static inline Mat4f getProjectionMatrixPerspective(float fov_degrees, float aspect_ratio, float z_near, float z_far, ZNearMapping z_near_mapping)
    {
      glm::mat4(*func)(float, float, float, float);
      if (z_near_mapping == ZNearMapping::MINUS_ONE) func = glm::perspectiveLH_NO;
      else func = glm::perspectiveLH_ZO;
      return func(glm::radians(fov_degrees), aspect_ratio, z_near, z_far);
    }
    static inline Mat4f getProjectionMatrixOrtho(const Vec3f& bb_min, const Vec3f& bb_max, ZNearMapping z_near_mapping)
    {
      glm::mat4(*func)(float, float, float, float, float, float);
      if (z_near_mapping == ZNearMapping::MINUS_ONE) func = glm::orthoLH_NO;
      else func = glm::orthoLH_ZO;
      return func(bb_min[0], bb_max[0], bb_min[1], bb_max[1], bb_min[2], bb_max[2]);
    }
    /**
    * Translates the camera to the origin, then rotates the coordinate system,
    * such that right, up and direction make up the new coordinate axes x, y and z.
    * The resulting matrix is used to transform vertices from world space to view space.
    */
    static inline Mat4f getViewMatrixLeftHanded(const Vec3f& pos, const Vec3f& right, const Vec3f& up, const Vec3f& direction)
    {
      return Mat4f({ Vec4f(right[0], up[0], direction[0], 0.f),
        Vec4f(right[1], up[1], direction[1], 0.f),
        Vec4f(right[2], up[2], direction[2], 0.f),
        Vec4f(-dot(right, pos), -dot(up, pos), -dot(direction, pos), 1.f) });
    }
  };
}

#endif
