#include <FixedTimestepSystem.h>
#include <GameTimer.h>
#include <iostream>

namespace fly
{
  void FixedTimestepSystem::update()
  {
    _acc += _gameTimer->getDeltaTimeSeconds();
    while (_acc >= _dt) {
      saveState();
      updateSystem();
      _acc -= _dt;
    }
    interpolate(_acc / _dt);
  }
}