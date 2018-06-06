#ifndef MATVECHELPERS_H
#define MATVECHELPERS_H

#include <math/FlyVector.h>
#include <math/FlyMatrix.h>
#include <math/Meta.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <array>

namespace fly
{
  template<unsigned Dim, typename T>
  static inline T dot(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return ComputeDot<Dim, T, Dim - 1>::call(a, b);
  }

  /**
  * Component-wise minimum
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> minimum(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    Vector<Dim, T> result;
    ComputeMin<Dim, T, Dim - 1>::call(a, b, result);
    return result;
  }
  /**
  * Component-wise maximum
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> maximum(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    Vector<Dim, T> result;
    ComputeMax<Dim, T, Dim - 1>::call(a, b, result);
    return result;
  }

  /**
  * Component-wise clamping
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> clamp(const Vector<Dim, T>& a, const Vector<Dim, T>& min_val, const Vector<Dim, T>& max_val)
  {
    return minimum(max_val, maximum(min_val, a));
  }

  template<unsigned Dim, typename T>
  static inline T distance(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return (b - a).length();
  }

  /**
  * Squared distance, doesn't involve expensive square root evaluation
  */
  template<unsigned Dim, typename T>
  static inline T distance2(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return (b - a).length2();
  }

  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> normalize(const Vector<Dim, T>& a)
  {
    return a / a.length();
  }
  /**
  * Component-wise round
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> round(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    ComputeRound<Dim, T, Dim - 1>::call(a, result);
    return result;
  }
  /**
  * Component-wise floor
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> floor(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    ComputeFloor<Dim, T, Dim - 1>::call(a, result);
    return result;
  }
  /**
  * Component-wise ceil
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> ceil(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    ComputeCeil<Dim, T, Dim - 1>::call(a, result);
    return result;
  }
  /**
  * Component-wise absolute value
  */
  template<unsigned Dim, typename T>
  static inline Vector<Dim, T> abs(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    ComputeAbs<Dim, T, Dim - 1>::call(a, result);
    return result;
  }
  /**
  * Identity matrix
  */
  template<unsigned Dim, typename T>
  static inline Matrix<Dim, Dim, T> identity()
  {
    Matrix<Dim, Dim, T> ret;
    for (unsigned i = 0; i < Dim; i++) {
      for (unsigned j = 0; j < Dim; j++) {
        ret[i][j] = i == j ? static_cast<T>(1) : static_cast<T>(0);
      }
    }
    return ret;
  }
  /**
  * Translation matrix
  */
  template<unsigned Dim, typename T>
  static inline Matrix<Dim, Dim, T> translate(const Vector<Dim - 1, T>& t)
  {
    auto ret = identity<Dim, T>();
    for (unsigned i = 0; i < Dim - 1; i++) {
      ret[Dim - 1][i] = t[i];
    }
    return ret;
  }

  /**
  * Scale matrix
  */
  template<unsigned Dim, typename T>
  static inline Matrix<Dim, Dim, T> scale(const Vector<Dim - 1, T>& s)
  {
    auto ret = identity<Dim, T>();
    for (unsigned i = 0; i < Dim - 1; i++) {
      ret[i][i] = s[i];
    }
    return ret;
  }

  /**
  * Matrix inverse
  */
  template<unsigned Dim, typename T>
  static inline Matrix<Dim, Dim, T> inverse(const Matrix<Dim, Dim, T>& mat)
  {
    return glm::inverse(glm::mat<Dim, Dim, T>(mat));
  }

  /**
  * Matrix transpose
  */
  template<unsigned Dim, typename T>
  static inline Matrix<Dim, Dim, T> transpose(const Matrix<Dim, Dim, T>& mat)
  {
    return glm::transpose(glm::mat<Dim, Dim, T>(mat));
  }
}

#endif
