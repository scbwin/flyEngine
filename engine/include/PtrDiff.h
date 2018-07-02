#ifndef PTRDIFF_H
#define PTRDIFF_H

#include <vector>

namespace fly
{
  namespace PtrDiff
  {
    template<typename SmartPtr>
    static float computeAveragePtrDiff(const std::vector<SmartPtr>& vec)
    {
      float diff = 0.f;
      for (unsigned i = 1; i < vec.size(); i++) {
        diff += (std::abs(reinterpret_cast<unsigned char*>(vec[i - 1].get()) - reinterpret_cast<unsigned char*>(vec[i].get()))) / static_cast<float>(vec.size());
      }
      return diff;
    }
  }
}

#endif