#ifndef FIXEDSTACKPOD_H
#define FIXEDSTACKPOD_H

#include <cstdlib>
#include <type_traits>

namespace fly
{
  template<typename T, size_t init_size = 1>
  class FixedStackPOD
  {
    static_assert(std::is_pod<T>::value, "T must be a POD type");
    static_assert(init_size > 0, "init_size must not be zero");

  public:
    FixedStackPOD()
    {
      resize(init_size);
      clear();
    }
    FixedStackPOD(const FixedStackPOD& other) = delete;
    FixedStackPOD& operator=(const FixedStackPOD& other) = delete;
    FixedStackPOD(FixedStackPOD&& other) = delete;
    FixedStackPOD& operator=(FixedStackPOD&& other) = delete;
    ~FixedStackPOD()
    {
      std::free(_begin);
    }
    void resize(size_t new_size)
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
    const T& operator[] (unsigned i) const
    {
      return _begin[i];
    }
    T& operator[] (unsigned i)
    {
      return _begin[i];
    }
    size_t capacity() const
    {
      return _capacity;
    }
  private:
    T * _begin = nullptr;
    T * _end;
    size_t _capacity;
  };
}
#endif