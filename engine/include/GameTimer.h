#ifndef GAMETIMER_H
#define GAMETIMER_H

#include <chrono>

namespace fly
{
  class GameTimer
  {
  public:
    GameTimer();
    void tick();
    void stop();
    void start();
    float getTimeSeconds() const;
    float getDeltaTimeSeconds() const;
    float getTotalTimeSeconds() const;
    bool isStopped() const;
  private:
    std::chrono::high_resolution_clock::time_point _baseTime;
    std::chrono::high_resolution_clock::time_point _currTime;
    std::chrono::high_resolution_clock::time_point _prevTime;
    std::chrono::high_resolution_clock::time_point _stopTime;
    std::chrono::nanoseconds _pausedTime = std::chrono::nanoseconds(0);
    std::chrono::nanoseconds _deltaTime = std::chrono::nanoseconds(0);
    bool _stopped = false;
  };
}

#endif // !GAMETIMER_H
