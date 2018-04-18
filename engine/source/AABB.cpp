#include <AABB.h>

namespace fly
{
  AABB::AABB(const Vec3f & bb_min, const Vec3f & bb_max, bool compute_vertices) :
    _bbMin(bb_min),
    _bbMax(bb_max), 
    _center((_bbMin + _bbMax) * 0.5f),
    _size(distance(_bbMin, _bbMax))
  {
    if (compute_vertices) {
      computeVertices();
    }
  }
  AABB::AABB(const AABB& aabb_local, const Mat4f & world_matrix) :
    _bbMin(std::numeric_limits<float>::max()),
    _bbMax(std::numeric_limits<float>::lowest())
  {
    unsigned i = 0;
    for (const auto& v : aabb_local._vertices) {
      auto v_world = (world_matrix * Vec4f(v, 1.f)).xyz();
      _bbMin = minimum(_bbMin, v_world);
      _bbMax = maximum(_bbMax, v_world);
      i++;
    }
    _center = (_bbMin + _bbMax) * 0.5f;
    _size = distance(_bbMin, _bbMax);
    computeVertices();
  }
  const Vec3f (& AABB::getVertices() const)[8]
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
  float AABB::size() const
  {
    return _size;
  }
  bool AABB::contains(const AABB & other) const
  {
    return _bbMin <= other._bbMin && _bbMax >= other._bbMax;
  }
  AABB AABB::getUnion(const AABB & other) const
  {
    return AABB(minimum(_bbMin, other._bbMin), maximum(_bbMax, other._bbMax));
  }
  AABB AABB::getIntersection(const AABB & other) const
  {
    return AABB(maximum(_bbMin, other._bbMin), minimum(_bbMax, other._bbMax));
  }
  void AABB::computeVertices()
  {
    _vertices[0] = Vec3f(_bbMin[0], _bbMin[1], _bbMin[2]);
    _vertices[1] = Vec3f(_bbMax[0], _bbMin[1], _bbMin[2]);
    _vertices[2] = Vec3f(_bbMin[0], _bbMax[1], _bbMin[2]);
    _vertices[3] = Vec3f(_bbMin[0], _bbMin[1], _bbMax[2]);
    _vertices[4] = Vec3f(_bbMax[0], _bbMax[1], _bbMin[2]);
    _vertices[5] = Vec3f(_bbMin[0], _bbMax[1], _bbMax[2]);
    _vertices[6] = Vec3f(_bbMax[0], _bbMin[1], _bbMax[2]);
    _vertices[7] = Vec3f(_bbMax[0], _bbMax[1], _bbMax[2]);
  }
}