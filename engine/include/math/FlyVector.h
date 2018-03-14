#ifndef FLYVECTOR_H
#define FLYVECTOR_H

#include <glm/glm.hpp>
#include <initializer_list>
#include <cstring>
#include <ostream>

namespace fly
{
  template<unsigned Dim, typename T>
  class Vector
  {
  public:
    /**
    * Constructors
    */
    Vector() = default;
    Vector(const std::initializer_list<T>& values)
    {
      std::memcpy(_data, values.begin(), sizeof *this);
    }
    Vector(const T* ptr)
    {
      std::memcpy(_data, ptr, sizeof(T) * Dim);
    }
    Vector(const T& value)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] = value;
      }
    }
    /**
    * Copy constructor
    */
    Vector(const Vector& other) = default;
    /**
    * Destructor
    */
    ~Vector() = default;
    /**
    * Member access
    */
    inline T& operator [] (unsigned index)
    {
      return _data[index];
    }
    inline const T& operator [] (unsigned index) const
    {
      return _data[index];
    }
    inline const T* ptr() const
    {
      return _data;
    }
    /**
    * Vector/vector calculations
    */
    inline Vector operator + (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] + b[i];
      }
      return result;
    }
    inline Vector operator - (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] - b[i];
      }
      return result;
    }
    inline Vector operator * (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] * b[i];
      }
      return result;
    }
    inline Vector operator / (const Vector& b) const
    {
      Vector result;
      for (unsigned i = 0; i < Dim; i++) {
        result[i] = _data[i] / b[i];
      }
      return result;
    }
    inline Vector& operator += (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] += b[i];
      }
      return *this;
    }
    inline Vector& operator -= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] -= b[i];
      }
      return *this;
    }
    inline Vector& operator *= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] *= b[i];
      }
      return *this;
    }
    inline Vector& operator /= (const Vector& b)
    {
      for (unsigned i = 0; i < Dim; i++) {
        _data[i] /= b[i];
      }
      return *this;
    }
    /**
    * Vector length
    */
    inline T length() const
    {
      return std::sqrt(dot(*this, *this));
    }
    /**
    * Conversion from/to glm
    */
    inline Vector(const glm::vec<Dim, T>& vec)
    {
      std::memcpy(_data, &vec[0], sizeof *this);
    }
    inline operator glm::vec<Dim, T>() const
    {
      glm::vec<Dim, T> vec;
      std::memcpy(&vec[0], _data, sizeof vec);
      return vec;
    }
    /**
    * Debug output
    */
    friend std::ostream& operator << (std::ostream& os, const Vector& vec)
    {
      os << "[";
      for (unsigned i = 0; i < Dim; i++) {
        os << vec._data[i];
        if (i != Dim - 1) {
          os << " ";
        }
      }
      os << "]";
      return os;
    }
  private:
    T _data[Dim];
  };
}

#endif
