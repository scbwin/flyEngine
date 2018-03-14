#ifndef FLYMATRIX_H
#define FLYMATRIX_H

#include <math/FlyVector.h>
#include <glm/glm.hpp>

namespace fly
{
  template<unsigned Rows, unsigned Cols, typename T>
  class Matrix
  {
  public:
    /**
    * Constructors
    */
    Matrix() = default;
    Matrix(const std::initializer_list<Vector<Rows, T>>& columns)
    {
      std::memcpy(_cols, columns.begin(), sizeof *this);
    }
    Matrix(const T* ptr)
    {
      std::memcpy(_cols, ptr, sizeof *this);
    }
    /**
    * Copy constructor
    */
    Matrix(const Matrix& other) = default;
    /**
    * Destructor
    */
    ~Matrix() = default;
    /**
    * Member access
    */
    inline Vector<Rows, T>& operator [] (unsigned col)
    {
      return _cols[col];
    }
    inline const Vector<Rows, T>& operator [] (unsigned col) const
    {
      return _cols[col];
    }
    inline Vector<Cols, T> row(unsigned row) const
    {
      Vector<Cols, T> vec;
      for (unsigned i = 0; i < Cols; i++) {
        vec[i] = _cols[i][row];
      }
      return vec;
    }
    inline const T* ptr() const
    {
      return &_cols[0][0];
    }
    /**
    * Matrix multiplication
    */
    template<unsigned N>
    inline Matrix<Rows, N, T> operator * (const Matrix<Cols, N, T>& b) const
    {
      T temp[Rows * N];
      for (unsigned row_idx = 0; row_idx < Rows; row_idx++) {
        auto row_vec = row(row_idx);
        for (unsigned col_idx = 0; col_idx < N; col_idx++) {
          temp[col_idx * Rows + row_idx] = dot(row_vec, b[col_idx]);
        }
      }
      return Matrix<Rows, N, T>(temp);
    }
    /**
    * Matrix/vector multiplication
    */
    inline Vector<Rows, T> operator * (const Vector<Rows, T>& vec) const
    {
      Vector<Rows, T> result;
      for (unsigned i = 0; i < Rows; i++) {
        result[i] = dot(row(i), vec);
      }
      return result;
    }
    /**
    * Conversion from/to glm
    */
    inline Matrix(const glm::mat<Cols, Rows, T>& mat)
    {
      std::memcpy(_cols, &mat[0][0], sizeof *this);
    }
    inline operator glm::mat<Cols, Rows, T>() const
    {
      glm::mat<Cols, Rows, T> mat;
      std::memcpy(&mat[0][0], _cols, sizeof mat);
      return mat;
    }
    /**
    * Comparison operators
    */
    inline bool operator != (const Matrix& b)
    {
      for (unsigned i = 0; i < Rows; i++) {
        for (unsigned j = 0; j < Cols; j++) {
          if (_cols[j][i] != b[j][i]) {
            return true;
          }
        }
      }
      return false;
    }

    /**
    * Debug output
    */
    friend std::ostream& operator << (std::ostream& os, const Matrix& mat)
    {
      os << "[";
      for (unsigned row = 0; row < Rows; row++) {
        for (unsigned col = 0; col < Cols; col++) {
          os << mat[col][row];
          if (col != Cols - 1) {
            os << " ";
          }
        }
        if (row != Rows - 1) {
          os << std::endl;
        }
      }
      os << "]";
      return os;
    }
  private:
    Vector<Rows, T> _cols[Cols];
  };
}

#endif
