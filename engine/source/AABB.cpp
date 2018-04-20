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
    _bbMin(aabb_local._bbMin),
    _bbMax(aabb_local._bbMax)
  {
    std::array<Vec3f, 8> vertices_world;
    for (unsigned i = 0; i < 8; i++) {
       vertices_world[i] = (world_matrix * Vec4f(getVertex(i), 1.f)).xyz();
    }
    _bbMin = Vec3f(std::numeric_limits<float>::max());
    _bbMax = Vec3f(std::numeric_limits<float>::lowest());
    for (unsigned i = 0; i < 8; i++) {
      _bbMin = minimum(_bbMin, vertices_world[i]);
      _bbMax = maximum(_bbMax, vertices_world[i]);
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
  bool AABB::intersects(const AABB & other) const
  {
    return _bbMax > other._bbMin && _bbMin < other._bbMax;
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