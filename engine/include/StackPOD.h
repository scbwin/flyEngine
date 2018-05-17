#ifndef STACKPOD_H
#define STACKPOD_H

#include <cstdlib>
#include <type_traits>

namespace fly
{
  /**
  * A lightweight alternative to std::vector for POD (Plain Old Data) types.
  */
  template<typename T, size_t init_size = 1>
  class StackPOD
  {
    static_assert(std::is_pod<T>::value, "T must be a POD type");
    static_assert(init_size > 0, "init_size must not be zero");

  public:
    StackPOD()
    {
      reserve(init_size);
      clear();
    }
    StackPOD(const StackPOD& other) = delete;
    StackPOD& operator=(const StackPOD& other) = delete;
    StackPOD(StackPOD&& other) = delete;
    StackPOD& operator=(StackPOD&& other) = delete;
    ~StackPOD()
    {
      std::free(_begin);
    }
    inline void reserve(size_t new_size)
    {
      _begin = reinterpret_cast<T*>(std::realloc(_begin, new_size * sizeof(T)));
      _capacity = new_size;
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
        auto size_old = size();
        reserve(capacity() * 2);
        _end = _begin + size_old;
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
    inline const T& operator[] (unsigned i) const
    {
      return _begin[i];
    }
    inline T& operator[] (unsigned i)
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
    T * _end;
    size_t _capacity;
  };
}
#endif