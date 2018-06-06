#ifndef META_H
#define META_H

#include <math/FlyVector.h>

namespace fly
{
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeDot
  {
    static inline T call(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
    {
      return a[index] * b[index] + ComputeDot<Dim, T, index - 1>::call(a, b);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeDot<Dim, T, 0>
  {
    static inline T call(const Vector<Dim, T>& a, const Vector<Dim, T>& b)
    {
      return a[0] * b[0];
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeMin
  {
    static inline void call(const Vector<Dim, T>& a, const Vector<Dim, T>& b, Vector<Dim, T>& result)
    {
      result[index] = std::min(a[index], b[index]);
      ComputeMin<Dim, T, index - 1>::call(a, b, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeMin<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, const Vector<Dim, T>& b, Vector<Dim, T>& result)
    {
      result[0] = std::min(a[0], b[0]);
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeMax
  {
    static inline void call(const Vector<Dim, T>& a, const Vector<Dim, T>& b, Vector<Dim, T>& result)
    {
      result[index] = std::max(a[index], b[index]);
      ComputeMax<Dim, T, index - 1>::call(a, b, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeMax<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, const Vector<Dim, T>& b, Vector<Dim, T>& result)
    {
      result[0] = std::max(a[0], b[0]);
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeRound
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[index] = std::round(a[index]);
      ComputeRound<Dim, T, index - 1>::call(a, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeRound<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[0] = std::round(a[0]);
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeFloor
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[index] = std::floor(a[index]);
      ComputeFloor<Dim, T, index - 1>::call(a, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeFloor<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[0] = std::floor(a[0]);
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeCeil
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[index] = std::ceil(a[index]);
      ComputeCeil<Dim, T, index - 1>::call(a, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeCeil<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[0] = std::ceil(a[0]);
    }
  };
  template<unsigned Dim, typename T, unsigned index>
  struct ComputeAbs
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[index] = std::abs(a[index]);
      ComputeAbs<Dim, T, index - 1>::call(a, result);
    }
  };
  template<unsigned Dim, typename T>
  struct ComputeAbs<Dim, T, 0>
  {
    static inline void call(const Vector<Dim, T>& a, Vector<Dim, T>& result)
    {
      result[0] = std::abs(a[0]);
    }
  };
}

#endif
