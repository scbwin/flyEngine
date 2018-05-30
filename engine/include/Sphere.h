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
    Sphere(const Vec3f& center, float radius);
    Sphere(const AABB& aabb);
    Sphere(Vec3f const * positions, size_t count);
    const Vec3f& getCenter() const;
    float getRadius() const;
  private:
    Vec3f _center = Vec3f(0.f);
    float _radius = 0.f;
  };
}

#endif // !SPHERE_H
