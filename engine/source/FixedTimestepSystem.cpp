#include <FixedTimestepSystem.h>
#include <GameTimer.h>

namespace fly
{
  void FixedTimestepSystem::update()
  {
    _acc += _gameTimer->getDeltaTimeSeconds();
    while (_acc >= _dt) {
      updateSystem();
      _acc -= _dt;
    }
  }
}