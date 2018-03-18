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