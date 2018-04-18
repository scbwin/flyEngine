#ifndef HELPERS_H
#define HELPERS_H

#include <math/FlyVector.h>
#include <math/FlyMatrix.h>
#include <math/Meta.h>
#include <algorithm>
#include <glm/glm.hpp>

namespace fly
{
  template<unsigned Dim, typename T>
  inline T dot(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return ComputeDot<Dim, T, Dim - 1>::call(a, b);
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> minimum(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    Vector<Dim, T> result;
    for (unsigned i = 0; i < Dim; i++) {
      result[i] = (std::min)(a[i], b[i]);
    }
    return result;
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> maximum(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    Vector<Dim, T> result;
    for (unsigned i = 0; i < Dim; i++) {
      result[i] = (std::max)(a[i], b[i]);
    }
    return result;
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> clamp(const Vector<Dim, T>& a, const Vector<Dim, T>& min_val, const Vector<Dim, T>& max_val)
  {
    return minimum(max_val, maximum(min_val, a));
  }

  template<unsigned Dim, typename T>
  inline T distance(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return (b - a).length();
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> normalize(const Vector<Dim, T>& a)
  {
    return a / a.length();
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> round(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    for (unsigned i = 0; i < Dim; i++) {
      result[i] = std::round(a[i]);
    }
    return result;
  }
  template<unsigned Dim, typename T>
  inline Vector<Dim, T> floor(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    for (unsigned i = 0; i < Dim; i++) {
      result[i] = std::floor(a[i]);
    }
    return result;
  }

  template<unsigned Dim, typename T>
  inline Vector<Dim, T> ceil(const Vector<Dim, T>& a)
  {
    Vector<Dim, T> result;
    for (unsigned i = 0; i < Dim; i++) {
      result[i] = std::ceil(a[i]);
    }
    return result;
  }

  template<unsigned Dim, typename T>
  inline Matrix<Dim, Dim, T> identity()
  {
    Matrix<Dim, Dim, T> ret;
    for (unsigned i = 0; i < Dim; i++) {
      for (unsigned j = 0; j < Dim; j++) {
        ret[i][j] = i == j ? static_cast<T>(1) : static_cast<T>(0);
      }
    }
    return ret;
  }

  template<unsigned Dim, typename T>
  inline Matrix<Dim, Dim, T> translate(const Vector<Dim - 1, T>& t)
  {
    auto ret = identity<Dim, T>();
    for (unsigned i = 0; i < Dim - 1; i++) {
      ret[Dim - 1][i] = t[i];
    }
    return ret;
  }

  template<unsigned Dim, typename T>
  inline Matrix<Dim, Dim, T> scale(const Vector<Dim - 1, T>& s)
  {
    auto ret = identity<Dim, T>();
    for (unsigned i = 0; i < Dim - 1; i++) {
      ret[i][i] = s[i];
    }
    return ret;
  }

  template<unsigned Dim, typename T>
  inline Matrix<Dim, Dim, T> inverse(const Matrix<Dim, Dim, T>& mat)
  {
    return glm::inverse(glm::mat<Dim, Dim, T>(mat));
  }
}

#endif // !HELPERS_H
