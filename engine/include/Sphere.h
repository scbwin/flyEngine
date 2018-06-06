#ifndef SPHERE_H
#define SPHERE_H

#include <math/FlyMath.h>
#include <ostream>

namespace fly
{
  class AABB;
  class Mesh;
  class Transform;

  class Sphere
  {
  public:
    Sphere() = default;
    Sphere(const Vec3f& center, float radius);
    Sphere(const AABB& aabb);
    Sphere(const Mesh& mesh);
    Sphere(const Sphere& other, const Transform& transform);
    const Vec3f& getCenter() const;
    float getRadius() const;
    friend std::ostream& operator << (std::ostream& os, const Sphere& sphere)
    {
      os << "Sphere [ " << sphere._center << " " << sphere._radius << " ]" << std::endl;
      return os;
    }
  private:
    Vec3f _center = Vec3f(0.f);
    float _radius = 0.f;
  };
}

#endif // !SPHERE_H
