#include <Sphere.h>
#include <AABB.h>
#include <Mesh.h>
#include <Transform.h>

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

  Sphere::Sphere(const Mesh & mesh)
  {
    for (const auto& v : mesh.getVertices()) {
      _center += v._position / static_cast<float>(mesh.getVertices().size());
    }
    for (const auto& v : mesh.getVertices()) {
      _radius = std::max(_radius, distance(v._position, _center));
    }
  }

  Sphere::Sphere(const Sphere & other, const Transform& transform) :
    _center(other._center + transform.getTranslation())
  {
    float scale = std::max(transform.getScale()[0], std::max(transform.getScale()[1], transform.getScale()[2]));
    _radius = other._radius * scale;
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