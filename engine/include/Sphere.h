#ifndef SPHERE_H
#define SPHERE_H

#include <math/FlyMath.h>

namespace fly
{
  class AABB;

  class Sphere
  {
  public:
    Sphere() = default;
    Sphere(const Vec3f& position, float radius);
    Sphere(const AABB& aabb);
    const Vec3f& getPosition() const;
    float getRadius() const;
  private:
    Vec3f _position = Vec3f(0.f);
    float _radius = 0.f;
  };
}

#endif // !SPHERE_H
