#ifndef VERTEX_H
#define VERTEX_H

#include <math/flyMath.h>

namespace fly
{
  struct CompressedNormal
  {
    int x : 10;
    int y : 10;
    int z : 10;
    int w : 2;
    CompressedNormal() = default;
    CompressedNormal(const Vec3f& vec)
    {
      Vec3i compressed(vec * 511.f);
      x = compressed[0];
      y = compressed[1];
      z = compressed[2];
      w = 1;
    }
  };
  struct Vertex
  {
    Vec3f _position;
    CompressedNormal _normal;
    Vec2f _uv;
    CompressedNormal _tangent;
    CompressedNormal _bitangent;
  };
}

#endif