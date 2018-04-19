#include <AABB.h>

namespace fly
{
  AABB::AABB(const Vec3f & bb_min, const Vec3f & bb_max) :
    _bbMin(bb_min),
    _bbMax(bb_max), 
    _size(distance(_bbMin, _bbMax))
  {
  }
  AABB::AABB(const AABB& aabb_local, const Mat4f & world_matrix) :
    _bbMin(std::numeric_limits<float>::max()),
    _bbMax(std::numeric_limits<float>::lowest())
  {
    for (unsigned i = 0; i < 8; i++) {
      auto v_world = (world_matrix * Vec4f(getVertex(i), 1.f)).xyz();
      _bbMin = minimum(_bbMin, v_world);
      _bbMax = maximum(_bbMax, v_world);
    }
    _size = distance(_bbMin, _bbMax);
  }
  const Vec3f& AABB::getMin() const
  {
    return _bbMin;
  }
  const Vec3f& AABB::getMax() const
  {
    return _bbMax;
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
  std::array<Vec3f, 8> AABB::getVertices() const
  {
    std::array<Vec3f, 8> vertices;
    for (unsigned i = 0; i < 8; i++) {
      vertices[i] = getVertex(i);
    }
    return vertices;
  }
}