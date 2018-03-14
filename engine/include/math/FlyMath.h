#ifndef FLYMATH_H
#define FLYMATH_H

#include <math/Helpers.h>
#include <math/FlyMatrix.h>

namespace fly
{
  using Vec2f = Vector<2, float>;
  using Vec3f = Vector<3, float>;
  using Vec4f = Vector<4, float>;

  using Vec2u = Vector<2, unsigned>;
  using Vec3u = Vector<3, unsigned>;
  using Vec4u = Vector<4, unsigned>;

  using Mat2f = Matrix<2, 2, float>;
  using Mat3f = Matrix<3, 3, float>;
  using Mat4f = Matrix<4, 4, float>;
}

#endif
