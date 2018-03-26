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
  private:
    std::chrono::high_resolution_clock::time_point _start;
  };
}

#endif