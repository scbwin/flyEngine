#ifndef AABB_H
#define AABB_H

#include <math/FlyMath.h>
#include <array>
#include <vector>
#include <iostream>
#include <Camera.h>

namespace fly
{
  class Mesh;
  struct Vertex;
  class Model;

  class AABB
  {
  public:
    AABB();
    AABB(const AABB& other);
    AABB& operator= (const AABB& other);
    AABB(const Vec3f& bb_min, const Vec3f& bb_max);
    AABB(const AABB& other, const Mat4f& transform);
    AABB(const Model& model);
    AABB(const Mesh& mesh);
    AABB(const std::vector<Vertex>& vertices);
    AABB(const Vec3f * first, size_t count);
    template<size_t count>
    static AABB fromTransform(const Vec3f * first, const Mat4f& transform)
    {
      Vec3f vertices[count];
      for (size_t i = 0; i < count; i++) {
        Vec4f vertex_h = transform * Vec4f(first[i], 1.f);
        vertices[i] = vertex_h.xyz() / vertex_h[3];
      }
      return AABB(vertices, count);
    }
    const Vec3f& getMin() const;
    const Vec3f& getMax() const;
    Vec3f& getMin();
    Vec3f& getMax();
    inline Vec3f center() const
    {
      return (_bbMin + _bbMax) * 0.5f;
    }
    inline float size() const
    {
      return distance(_bbMin, _bbMax);
    }
    inline float size2() const
    {
      return distance2(_bbMin, _bbMax);
    }
    bool contains(const AABB& other) const;
    bool intersects(const AABB& other) const;
    AABB getUnion(const AABB& other) const;
    AABB getIntersection(const AABB& other) const;
    std::array<Vec3f, 8> getVertices() const;
    void expand(const Vec3f& amount);
    inline bool isLargeEnough(const Vec3f& cam_pos, float tresh, float size) const
    {
      return (size / distance2(closestPoint(cam_pos), cam_pos)) > tresh;
    }
    inline bool isLargeEnough(const Vec3f& cam_pos, float error_tresh) const
    {
      return isLargeEnough(cam_pos, error_tresh, size2());
    }
    inline Vec3f closestPoint(const Vec3f& point) const
    {
      return clamp(point, _bbMin, _bbMax);
    }
    /*
    * Returns the aabb vertex for an index between 0 and 7, the result is
    * undefined if the index is not between 0 and 7.
    */
    inline Vec3f getVertex(unsigned char index) const
    {
      return Vec3f(index & 4 ? _bbMin[0] : _bbMax[0],
        index & 2 ? _bbMin[1] : _bbMax[1], 
        index & 1 ? _bbMin[2] : _bbMax[2]);
    }

    friend std::ostream& operator << (std::ostream& os, const AABB& aabb)
    {
      os << "AABB [ " << aabb.getMin() << " " << aabb.getMax() << " ]" << std::endl;
      return os;
    }
  private:
    Vec3f _bbMin, _bbMax;
  };
}

#endif // !AABB_H
