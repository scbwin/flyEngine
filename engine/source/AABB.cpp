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

    _center = (_bbMin + _bbMax) * 0.5f;
  }
  AABB::AABB(const AABB& aabb_local, const Mat4f & world_matrix) :
    _bbMin(std::numeric_limits<float>::max()),
    _bbMax(std::numeric_limits<float>::lowest())
  {
    unsigned i = 0;
    for (const auto& v : aabb_local._vertices) {
      _vertices[i] = world_matrix * Vec4f({ v[0], v[1], v[2], 1.f });
      _bbMin = minimum(_bbMin, _vertices[i]);
      _bbMax = maximum(_bbMax, _vertices[i]);
      i++;
    }
    _center = (_bbMin + _bbMax) * 0.5f;
  }
  AABB::AABB(const AABB & other) : 
    _vertices(other._vertices), 
    _bbMin(other._bbMin), 
    _bbMax(other._bbMax), 
    _center(other._center)
  {
  }
  const std::array<Vec3f, 8>& AABB::getVertices() const
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
  const Vec3f& AABB::center() const
  {
    return _center;
  }
}