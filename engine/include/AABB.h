#ifndef AABB_H
#define AABB_H

#include <math/FlyMath.h>
#include <array>
#include <vector>
#include <iostream>

namespace fly
{
  class AABB
  {
  public:
    AABB(const Vec3f& bb_min, const Vec3f& bb_max);
    AABB(const AABB& aabb_local, const Mat4f& world_matrix);
    const Vec3f* getVertices() const;
    const Vec3f& getMin() const;
    const Vec3f& getMax() const;
    const Vec3f& center() const;

    template<bool directx>
    inline bool isFullyVisible(const Mat4f& transform)
    {
      for (const auto& v : _vertices) {
        auto pos_h = transform * Vec4f(v, 1.f);
        for (unsigned i = 0; i < 3; i++) {
          if (pos_h[i] > pos_h[3]) {
            return false;
          }
        }
        for (unsigned i = 0; i < 3; i++) {
          if (pos_h[i] < (directx && i == 2 ? 0.f : -pos_h[3])) {
            return false;
          }
        }
      }
      return true;
    }

    template<bool directx>
    inline bool intersectsFrustum(const Mat4f& transform)
    {
      unsigned char intersects = 0;
      for (const auto& v : _vertices) {
        auto pos_h = transform * Vec4f(v, 1.f);
        for (unsigned i = 0; i < 3; i++) {
          intersects |= (pos_h[i] <= pos_h[3]) << i;
        }
        for (unsigned i = 0; i < 3; i++) {
          intersects |= (pos_h[i] >= (directx && i == 2 ? 0.f : -pos_h[3])) << (i + 3);
        }
        if (intersects == 0b00111111) {
          return true;
        }
      }
      return false;
    }

    template<bool directx, bool ignore_near>
    inline bool isVisible(const Mat4f& mvp) const
    {
      bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_far = false, inside_near;
      if (!ignore_near) {
        inside_near = false;
      }
      for (const auto& v : _vertices) {
        auto pos_h = mvp * Vec4f( v[0], v[1], v[2], 1.f );
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
    template<bool directx, bool ignore_near>
    inline bool isVisible(const std::vector<Mat4f>& vps) const
    {
      bool visible = false;
      for (const auto& vp : vps) {
        visible = visible || isVisible<directx, ignore_near>(vp);
      }
      return visible;
    }
  private:
    Vec3f _vertices [8];
    Vec3f _bbMin, _bbMax;
    Vec3f _center;
  };
}

#endif // !AABB_H
