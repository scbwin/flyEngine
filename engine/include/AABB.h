#ifndef AABB_H
#define AABB_H

#include <math/FlyMath.h>
#include <array>

namespace fly
{
  class AABB
  {
  public:
    AABB(const Vec3f& bb_min, const Vec3f& bb_max);
    bool isVisible(const Mat4f& mvp, bool directx) const;
    std::array<Vec3f, 8>& getVertices();
    const Vec3f& getMin() const;
    const Vec3f& getMax() const;
  private:
    std::array<Vec3f, 8> _vertices;
    Vec3f _bbMin, _bbMax;
  };
}

#endif // !AABB_H
