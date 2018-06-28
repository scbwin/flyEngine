#ifndef CULLRESULT_H
#define CULLRESULT_H

#include <StackPOD.h>

namespace fly
{
  template<typename T>
  struct CullResult
  {
    StackPOD<T> _fullyVisibleObjects;
    StackPOD<T> _probablyVisibleObjects;
    inline void reserve(size_t size)
    {
      _fullyVisibleObjects.reserve(size);
      _probablyVisibleObjects.reserve(size);
    }
    inline void clear()
    {
      _fullyVisibleObjects.clear();
      _probablyVisibleObjects.clear();
    }
    inline size_t capacity()
    {
      return _fullyVisibleObjects.capacity() + _probablyVisibleObjects.capacity();
    }
    inline size_t size()
    {
      return _fullyVisibleObjects.size() + _probablyVisibleObjects.size();
    }
  };
}

#endif // !CULLRESULT_H
