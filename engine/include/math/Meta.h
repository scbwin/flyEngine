#ifndef META_H
#define META_H

#include <math/FlyVector.h>

namespace fly
{
  /**
  * Recursive definition of the dot product, profiling revealed that this
  * is a lot faster than using a for loop.
  */
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeDot
  {
    static inline T call(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
    {
      return a[index] * b[index] + ComputeDot<Dim, T, index - 1>::call(a, b);
    }
  };
  // Termination criterion
  template<unsigned Dim, typename T>
  struct ComputeDot<Dim, T, 0>
  {
    static inline T call(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
    {
      return a[0] * b[0];
    }
  };
}

#endif
