#include <Sphere.h>
#include <AABB.h>

namespace fly
{
  Sphere::Sphere(const Vec3f & center, float radius) : 
    _center(center), 
    _radius(radius)
  {
  }
  Sphere::Sphere(const AABB & aabb) :
    _center(aabb.center())
  {
    for (const auto& v : aabb.getVertices()) {
      _radius = std::max(_radius, distance(_center, v));
    }
  }
  Sphere::Sphere(Vec3f const * positions, size_t count) :
    _center(0.f)
  {
    for (size_t i = 0; i < count; i++) {
      _center += positions[i] / static_cast<float>(count);
    }
    for (size_t i = 0; i < count; i++) {
      _radius = std::max(_radius, distance(_center, positions[i]));
    }
  }
  const Vec3f & Sphere::getCenter() const
  {
    return _center;
  }
  float Sphere::getRadius() const
  {
    return _radius;
  }
}