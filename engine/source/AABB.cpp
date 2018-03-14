#include <AABB.h>

namespace fly
{
  AABB::AABB(const Vec3f & bb_min, const Vec3f & bb_max) : _bbMin(bb_min), _bbMax(bb_max)
  {
    _vertices[0] = Vec3f({ bb_min[0], bb_min[1], bb_min[2] });
    _vertices[1] = Vec3f({ bb_max[0], bb_min[1], bb_min[2] });
    _vertices[2] = Vec3f({ bb_min[0], bb_max[1], bb_min[2] });
    _vertices[3] = Vec3f({ bb_min[0], bb_min[1], bb_max[2] });
    _vertices[4] = Vec3f({ bb_max[0], bb_max[1], bb_min[2] });
    _vertices[5] = Vec3f({ bb_min[0], bb_max[1], bb_max[2] });
    _vertices[6] = Vec3f({ bb_max[0], bb_min[1], bb_max[2] });
    _vertices[7] = Vec3f({ bb_max[0], bb_max[1], bb_max[2] });
  }
  bool AABB::isVisible(const Mat4f& mvp, bool directx) const
  {
    bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_near = false, inside_far = false;
    for (const auto& v : _vertices) {
      auto pos_h = mvp * Vec4f({ v[0], v[1], v[2], 1.f });
      inside_left = inside_left || pos_h[0] >= -pos_h[3];
      inside_right = inside_right || pos_h[0] <= pos_h[3];
      inside_bottom = inside_bottom || pos_h[1] >= -pos_h[3];
      inside_top = inside_top || pos_h[1] <= pos_h[3];
      inside_near = inside_near || pos_h[2] >= (directx ? 0.f : -pos_h[3]);
      inside_far = inside_far || pos_h[2] <= pos_h[3];
    }
    return inside_left && inside_right && inside_bottom && inside_top && inside_near && inside_far;
  }
  std::array<Vec3f, 8>& AABB::getVertices()
  {
    return _vertices;
  }
  const Vec3f& AABB::getMin() const
  {
    return _bbMin;
  }
  const Vec3f& AABB::getMax() const
  {
    return _bbMax;
  }
}