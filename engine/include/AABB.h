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
    std::array<Vec3f, 8>& getVertices();
    const Vec3f& getMin() const;
    const Vec3f& getMax() const;

    template<bool directx, bool ignore_near>
    inline bool isVisible(const Mat4f& mvp) const
    {
      bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_far = false, inside_near;
      if (!ignore_near) {
        inside_near = false;
      }
      for (const auto& v : _vertices) {
        auto pos_h = mvp * Vec4f({ v[0], v[1], v[2], 1.f });
        inside_left = inside_left || pos_h[0] >= -pos_h[3];
        inside_right = inside_right || pos_h[0] <= pos_h[3];
        inside_bottom = inside_bottom || pos_h[1] >= -pos_h[3];
        inside_top = inside_top || pos_h[1] <= pos_h[3];
        if (!ignore_near) {
          inside_near = inside_near || pos_h[2] >= (directx ? 0.f : -pos_h[3]);
        }
        inside_far = inside_far || pos_h[2] <= pos_h[3];
      }
      bool is_visible = inside_left && inside_right && inside_bottom && inside_top && inside_far;
      return ignore_near ? is_visible : is_visible && inside_near;
    }
  private:
    std::array<Vec3f, 8> _vertices;
    Vec3f _bbMin, _bbMax;
  };
}

#endif // !AABB_H
