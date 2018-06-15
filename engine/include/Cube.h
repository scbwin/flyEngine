#ifndef CUBE_H
#define CUBE_H

#include <math/FlyMath.h>

namespace fly
{
  class Cube
  {
  public:
    Cube(const Vec3f& min, float size);
    const Vec3f& getMin() const;
    float getSize() const;
    Vec3f getVertex(unsigned char i) const;
  private:
    Vec3f _min;
    float _size;
    std::array<fly::Vec3f, 8> _vertices;
  };
}

#endif // !CUBE_H
