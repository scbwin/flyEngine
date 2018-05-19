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
    StackPOD(const StackPOD& other) = delete;
    StackPOD& operator=(const StackPOD& other) = delete;
    StackPOD(StackPOD&& other) = delete;
    StackPOD& operator=(StackPOD&& other) = delete;
    ~StackPOD()
    {
      std::free(_begin);
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