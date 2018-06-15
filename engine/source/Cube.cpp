#include <Cube.h>

namespace fly
{
  Cube::Cube(const Vec3f & min, float size) : _min(min), _size(size)
  {
    _vertices[0] = _min;
    _vertices[1] = Vec3f(min[0] + size, min[1], min[2]);
    _vertices[2] = Vec3f(min[0] + size, min[1], min[2] + size);
    _vertices[3] = Vec3f(min[0], min[1], min[2] + size);
    _vertices[4] = Vec3f(min[0], min[1] + size, min[2]);
    _vertices[5] = Vec3f(min[0] + size, min[1] + size, min[2]);
    _vertices[6] = Vec3f(min[0] + size, min[1] + size, min[2] + size);
    _vertices[7] = Vec3f(min[0], min[1] + size, min[2] + size);
  }
  const Vec3f & Cube::getMin() const
  {
    return _min;
  }
  float Cube::getSize() const
  {
    return _size;
  }
  Vec3f Cube::getVertex(unsigned char i) const
  {
    return _vertices[i];
    //return Vec3f(i & 1 ? _min[0] : _min[0] + _size,
    //  i & 4 ? _min[1] : _min[1] + _size,
    //  i & 2 ? _min[2] : _min[2] + _size);
  }
}