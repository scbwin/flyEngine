#include <Sphere.h>
#include <AABB.h>

namespace fly
{
  Sphere::Sphere(const Vec3f & position, float radius) : 
    _position(position), 
    _radius(radius)
  {
  }
  Sphere::Sphere(const AABB & aabb) :
    _position(aabb.center())
  {
    for (const auto& v : aabb.getVertices()) {
      _radius = std::max(_radius, distance(aabb.center(), v));
    }
  }
  const Vec3f & Sphere::getPosition() const
  {
    return _position;
  }
  float Sphere::getRadius() const
  {
    return _radius;
  }
}