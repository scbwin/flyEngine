#include <AABB.h>
#include <Mesh.h>
#include <Model.h>

namespace fly
{
  AABB::AABB() :
    _bbMin(std::numeric_limits<float>::max()),
    _bbMax(std::numeric_limits<float>::lowest())
  {
  }
  AABB::AABB(const AABB & other) :
    _bbMin(other._bbMin),
    _bbMax(other._bbMax)
  {
  }
  AABB& AABB::operator=(const AABB & other)
  {
    _bbMin = other._bbMin;
    _bbMax = other._bbMax;
    return *this;
  }
  AABB::AABB(const Vec3f & bb_min, const Vec3f & bb_max) :
    _bbMin(bb_min),
    _bbMax(bb_max)
  {
  }
  AABB::AABB(const AABB& other, const Mat4f & transform) :
    AABB()
  {
    for (unsigned i = 0; i < 8; i++) {
      _bbMin = minimum(_bbMin, (transform * Vec4f(other.getVertex(i), 1.f)).xyz());
      _bbMax = maximum(_bbMax, (transform * Vec4f(other.getVertex(i), 1.f)).xyz());
    }
  }
  AABB::AABB(const Model & model) :
    AABB()
  {
    for (const auto& m : model.getMeshes()) {
      *this = getUnion(m->getAABB());
    }
  }
  AABB::AABB(const Mesh & mesh) : 
    AABB(mesh.getVertices())
  {
  }
  AABB::AABB(const std::vector<Vertex>& vertices) :
    AABB()
  {
    for (const auto& v : vertices) {
      _bbMin = minimum(_bbMin, v._position);
      _bbMax = maximum(_bbMax, v._position);
    }
  }
  AABB::AABB(const Vec3f * first, size_t count) :
    AABB()
  {
    for (size_t i = 0; i < count; i++) {
      _bbMin = minimum(_bbMin, first[i]);
      _bbMax = maximum(_bbMax, first[i]);
    }
  }
  const Vec3f& AABB::getMin() const
  {
    return _bbMin;
  }
  const Vec3f& AABB::getMax() const
  {
    return _bbMax;
  }
  Vec3f & AABB::getMin()
  {
    return _bbMin;
  }
  Vec3f & AABB::getMax()
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
  void AABB::expand(const Vec3f & amount)
  {
    _bbMin -= amount;
    _bbMax += amount;
  }
}