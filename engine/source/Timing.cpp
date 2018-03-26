#include <Timing.h>
#include <iostream>

namespace fly
{
  Timing::Timing()
  {
    start();
  }
  void Timing::start()
  {
    _start = std::chrono::high_resolution_clock::now();
  }
  std::ostream& operator<<(std::ostream & os, const Timing & timing)
  {
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - timing._start).count();
    os << duration_ms << " ms";
    return os;
  }
}