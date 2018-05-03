#include <AABB.h>

namespace fly
{
  AABB::AABB(const Vec3f & bb_min, const Vec3f & bb_max) :
    _bbMin(bb_min),
    _bbMax(bb_max)
   // _size(distance2(bb_min, bb_max)),
   // _center((bb_min + bb_max) * 0.5f)
  {
  }
  AABB::AABB(const AABB& aabb_local, const Mat4f & world_matrix)
  {
    _bbMin = Vec3f(std::numeric_limits<float>::max());
    _bbMax = Vec3f(std::numeric_limits<float>::lowest());
    for (unsigned i = 0; i < 8; i++) {
      _bbMin = minimum(_bbMin, (world_matrix * Vec4f(aabb_local.getVertex(i), 1.f)).xyz());
      _bbMax = maximum(_bbMax, (world_matrix * Vec4f(aabb_local.getVertex(i), 1.f)).xyz());
    }
  //  _size = distance2(_bbMin, _bbMax);
   // _center = (_bbMin + _bbMax) * 0.5f;
  }
  const Vec3f& AABB::getMin() const
  {
    return _bbMin;
  }
  const Vec3f& AABB::getMax() const
  {
    return _bbMax;
  }
 /* Vec3f AABB::center() const
  {
    return (_bbMin + _bbMax) * 0.5f;
  }
  float AABB::size() const
  {
    return distance2(_bbMin, _bbMax);
  }*/
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