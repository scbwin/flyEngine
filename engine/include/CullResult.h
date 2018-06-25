#ifndef CULLRESULT_H
#define CULLRESULT_H

#include <StackPOD.h>

namespace fly
{
  template<typename T>
  struct CullResult
  {
    StackPOD<T> _fullyVisibleObjects;
    StackPOD<T> _intersectedObjects;
    inline void reserve(size_t size)
    {
      _fullyVisibleObjects.reserve(size);
      _intersectedObjects.reserve(size);
    }
    inline void clear()
    {
      _fullyVisibleObjects.clear();
      _intersectedObjects.clear();
    }
    inline size_t capacity()
    {
      return _fullyVisibleObjects.capacity() + _intersectedObjects.capacity();
    }
    inline size_t size()
    {
      return _fullyVisibleObjects.size() + _intersectedObjects.size();
    }
  };
}

#endif // !CULLRESULT_H
