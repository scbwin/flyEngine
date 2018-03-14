#ifndef VERTEX_H
#define VERTEX_H

#include <math/flyMath.h>

namespace fly
{
  struct Vertex
  {
    Vec3f _position;
    Vec3f _normal;
    Vec2f _uv;
    Vec3f _tangent;
    Vec3f _bitangent;
  };
}

#endif