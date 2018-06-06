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

  Sphere::Sphere(const Mesh & mesh, const Transform & transform)
  {
    auto vertices = mesh.getVertices();
    for (auto& v : vertices) {
      v._position = (transform.getModelMatrix() * Vec4f(v._position, 1.f)).xyz();
    }
    for (const auto& v : vertices) {
      _center += v._position / static_cast<float>(vertices.size());
    }
    for (const auto& v : vertices) {
      _radius = std::max(_radius, distance(v._position, _center));
    }
  }

  Sphere::Sphere(const Sphere & other, const Transform& transform) :
    _center((transform.getModelMatrix() * Vec4f(other._center, 1.f)).xyz())
  {
    float scale = std::max(transform.getScale()[0], std::max(transform.getScale()[1], transform.getScale()[2]));
    _radius = other._radius * scale;
  }

  Sphere Sphere::getUnion(const Sphere & other)
  {
    //return _radius == 0.f ? other : Sphere((_center + other._center) * 0.5f, (distance(_center, other._center) + _radius + other._radius) * 0.5f);

    // TODO: Implementation comes here
    return Sphere();
  }

  const Vec3f & Sphere::center() const
  {
    return _center;
  }
  float Sphere::radius() const
  {
    return _radius;
  }
  Vec3f Sphere::getMax() const
  {
    return _center + _radius;
  }
  Vec3f Sphere::getMin() const
  {
    return _center - _radius;
  }
  void Sphere::expand(const Vec3f & amount)
  {
    _radius += amount[0];
  }
}