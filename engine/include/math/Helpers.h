#ifndef HELPERS_H
#define HELPERS_H

#include <math/FlyVector.h>
#include <math/Meta.h>
#include <algorithm>

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
  inline T distance(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
  {
    return (b - a).length();
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
}

#endif // !HELPERS_H
