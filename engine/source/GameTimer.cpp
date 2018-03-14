#include <GameTimer.h>

namespace fly
{
  GameTimer::GameTimer()
  {
    _baseTime = std::chrono::high_resolution_clock::now();
    _prevTime = _baseTime;
  }
  void GameTimer::tick()
  {
    _currTime = std::chrono::high_resolution_clock::now();
    _deltaTime = _currTime - _prevTime;
    _prevTime = _currTime;
  }
  void GameTimer::stop()
  {
    if (!_stopped) {
      _stopTime = std::chrono::high_resolution_clock::now();
      _stopped = true;
    }
  }
  void GameTimer::start()
  {
    _currTime = std::chrono::high_resolution_clock::now();
    if (_stopped) {
      _pausedTime += _currTime - _stopTime;
      _prevTime = _currTime;
      _stopped = false;
    }
  }
  float GameTimer::getTimeSeconds()
  {
    if (_stopped) {
      return (_stopTime - _baseTime - _pausedTime).count() / 1000000000.f;
    }
    else {
      return (_currTime - _baseTime - _pausedTime).count() / 1000000000.f;
    }
  }
  float GameTimer::getDeltaTimeSeconds()
  {
    return _stopped ? 0.f : _deltaTime.count() / 1000000000.f;
  }
  float GameTimer::getTotalTimeSeconds()
  {
    return (_currTime - _baseTime).count() / 1000000000.f;
  }
  bool GameTimer::isStopped()
  {
    return _stopped;
  }
}