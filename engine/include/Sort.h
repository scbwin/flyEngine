#ifndef SORT_H
#define SORT_H

#include <algorithm>
#include <functional>

namespace fly
{
  template<typename T>
  static void bubbleSort(T* begin, T* end, const std::function<bool(const T&, const T&)>& comp)
  {
    for (T const * i = end; i > begin + 1; i--) {
      for (auto j = begin; j < i - 1; j++) {
        if (comp(*j, *(j + 1))) {
          std::swap(*j, *(j + 1));
        }
      }
    }
  }
}

#endif // !SORT_H
