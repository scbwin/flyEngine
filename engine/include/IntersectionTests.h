#ifndef INTERSECTIONTESTS_H
#define INTERSECTIONTESTS_H

#include <AABB.h>
#include <Sphere.h>
#include <array>

namespace fly
{
  enum class IntersectionResult
  {
    OUTSIDE, INSIDE, INTERSECTING
  };

  namespace IntersectionTests
  {
    static inline IntersectionResult planeIntersectsAABB(const Vec4f& plane, const Vec3f& half_diagonal, const Vec4f& center)
    {
      auto e = dot(half_diagonal, abs(plane.xyz()));
      auto s = dot(center, plane);
      if (s - e > 0.f) {
        return IntersectionResult::OUTSIDE;
      }
      return s + e < 0.f ? IntersectionResult::INSIDE : IntersectionResult::INTERSECTING;
    }
    static inline IntersectionResult planeIntersectsSphere(const Vec4f& plane, const Sphere& sphere)
    {
      float dist = dot(Vec4f(sphere.center(), 1.f), plane);
      if (dist > sphere.radius()) {
        return IntersectionResult::OUTSIDE;
      }
      return dist < -sphere.radius() ? IntersectionResult::INSIDE : IntersectionResult::INTERSECTING;
    }
    static inline bool aabbOutsideFrustum(const Vec4f& plane, const Vec3f& half_diagonal, const Vec4f& center)
    {
      auto e = dot(half_diagonal, abs(plane.xyz()));
      auto s = dot(center, plane);
      return s - e > 0.f;
    }
    static inline bool boundingVolumeOutsideFrustum(const AABB& aabb, const std::array<Vec4f, 6>& frustum_planes)
    {
      auto half_diagonal = (aabb.getMax() - aabb.getMin()) * 0.5f;
      Vec4f center(aabb.center(), 1.f);
      for (const auto& p : frustum_planes) {
        if (aabbOutsideFrustum(p, half_diagonal, center)) {
          return true;
        }
      }
      return false;
    }
    static inline IntersectionResult frustumIntersectsBoundingVolume(const AABB& aabb, const std::array<Vec4f, 6>& frustum_planes)
    {
      bool intersecting = false;
      auto half_diagonal = (aabb.getMax() - aabb.getMin()) * 0.5f;
      Vec4f center(aabb.center(), 1.f);
      for (const auto& p : frustum_planes) {
        auto result = planeIntersectsAABB(p, half_diagonal, center);
        if (result == IntersectionResult::OUTSIDE) {
          return IntersectionResult::OUTSIDE;
        }
        else if (result == IntersectionResult::INTERSECTING) {
          intersecting = true;
        }
      }
      return intersecting ? IntersectionResult::INTERSECTING : IntersectionResult::INSIDE;
    }
    static inline IntersectionResult frustumIntersectsBoundingVolume(const Sphere& sphere, const std::array<Vec4f, 6>& frustum_planes)
    {
      bool intersecting = false;
      for (const auto& p : frustum_planes) {
        auto result = planeIntersectsSphere(p, sphere);
        if (result == IntersectionResult::OUTSIDE) {
          return IntersectionResult::OUTSIDE;
        }
        else if (result == IntersectionResult::INTERSECTING) {
          intersecting = true;
        }
      }
      return intersecting ? IntersectionResult::INTERSECTING : IntersectionResult::INSIDE;
    }
  }
}

#endif // !INTERSECTIONTESTS_H
