#ifndef STACKPOD_H
#define STACKPOD_H

#include <cstdlib>
#include <type_traits>

namespace fly
{
  /**
  * A lightweight alternative to std::vector for POD (Plain Old Data) types.
  */
  template<typename T, size_t initial_capacity = 0>
  class StackPOD
  {
    static_assert(std::is_pod<T>::value, "T must be a POD type");

  public:
    StackPOD()
    {
      if (initial_capacity) {
        reserve(initial_capacity);
      }
    }
    StackPOD(size_t size)
    {
      allocate(size);
      _end = _begin + size;
    }
    StackPOD(const StackPOD& other)
    {
      allocate(other._capacity);
      std::memcpy(_begin, other._begin, other.size() * sizeof(T));
      _end = _begin + other.size();
    }
    StackPOD& operator=(const StackPOD& other)
    {
      if (_begin) {
        std::free(_begin);
      }
      _capacity = 0;
      allocate(other._capacity);
      std::memcpy(_begin, other._begin, other.size() * sizeof(T));
      _end = _begin + other.size();
      return *this;
    }
    StackPOD(StackPOD&& other) :
      _begin(other._begin),
      _end(other._end),
      _capacity(other._capacity)
    {
      other._begin = nullptr;
      other._end = nullptr;
      other._capacity = 0;
    }
    StackPOD& operator=(StackPOD&& other)
    {
      if (_begin) {
        std::free(_begin);
      }
      _begin = other._begin;
      _end = other._end;
      _capacity = other._capacity;

      other._begin = nullptr;
      other._end = nullptr;
      other._capacity = 0;
      return *this;
    }
    ~StackPOD()
    {
      if (_begin) {
        std::free(_begin);
      }
    }
    inline void append(const StackPOD& other)
    {
      auto new_size = size() + other.size();
      while (capacity() < new_size) {
        allocate(capacity() * 2u);
      }
      std::memcpy(_end, other._begin, other.size() * sizeof(T));
      _end += other.size();
    }
    inline void reserve(size_t new_capacity)
    {
      if (new_capacity > _capacity) {
        allocate(new_capacity);
      }
    }
    inline void clear()
    {
      _end = _begin;
    }
    /**
    * Adds a new element to the end of the stack. No range check is performed.
    */
    inline void push_back(const T& element)
    {
      *_end++ = element;
    }
    /**
    * Adds a new element to the end of the stack. If the current capacity is not sufficient,
    * a reallocation is performed that may invalidate all references and iterators.
    */
    inline void push_back_secure(const T& element)
    {
      if (size() == _capacity) {
        allocate(_capacity == 0 ? 1 : _capacity * 2);
      }
      push_back(element);
    }
    inline T* begin() const
    {
      return _begin;
    }
    inline T* end() const
    {
      return _end;
    }
    inline size_t size() const
    {
      return _end - _begin;
    }
    inline const T& operator[] (size_t i) const
    {
      return _begin[i];
    }
    inline T& operator[] (size_t i)
    {
      return _begin[i];
    }
    inline size_t capacity() const
    {
      return _capacity;
    }
    inline T& back()
    {
      return *(_end - 1);
    }
    inline const T& back() const
    {
      return *(_end - 1);
    }
    inline T const * find(const T& element) const
    {
      for (const auto& e : *this) {
        if (e == element) {
          return &e;
        }
      }
      return _end;
    }
  private:
    T * _begin = nullptr;
    T * _end = nullptr;
    size_t _capacity = 0;

    inline void allocate(size_t new_capacity)
    {
      size_t size_old = size();
      _begin = reinterpret_cast<T*>(std::realloc(_begin, new_capacity * sizeof(T)));
      _capacity = new_capacity;
      _end = _begin + size_old;
    }
  };
}
#endif