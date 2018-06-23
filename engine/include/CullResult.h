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
  };
}

#endif // !CULLRESULT_H
