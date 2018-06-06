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
    Sphere(const Mesh& mesh, const Transform& transform);
    Sphere(const Sphere& other, const Transform& transform);
    Sphere getUnion(const Sphere& other);
    const Vec3f& center() const;
    Vec3f getMax() const;
    Vec3f getMin() const;
    float radius() const;
    void expand(const Vec3f& amount);
    inline float size() const
    {
      return _radius * 2.f;
    }
    inline float size2() const
    {
      return _radius * 2.f * _radius * 2.f;
    }
    inline bool isLargeEnough(const Vec3f& cam_pos, float tresh, float size) const
    {
      return (size / distance2(_center + normalize(cam_pos - _center) * _radius, cam_pos)) > tresh;
    }
    inline bool isLargeEnough(const Vec3f& cam_pos, float error_tresh) const
    {
      return isLargeEnough(cam_pos, error_tresh, size2());
    }
    inline unsigned getLongestAxis(unsigned depth) const
    {
      return depth % 3;
    }
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
