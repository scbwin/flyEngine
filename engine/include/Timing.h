#ifndef TIMING_H
#define TIMING_H

#include <chrono>

namespace fly
{
  class Timing
  {
  public:
    Timing();
    void start();
    friend std::ostream& operator <<(std::ostream& os, const Timing& timing);
    template<typename T>
    unsigned duration() const
    {
      return std::chrono::duration_cast<T>(std::chrono::high_resolution_clock::now() - _start).count();
    }
  private:
    std::chrono::high_resolution_clock::time_point _start;
  };
}

#endif