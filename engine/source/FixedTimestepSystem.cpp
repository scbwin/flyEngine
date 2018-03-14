#include <FixedTimestepSystem.h>

namespace fly
{
  void FixedTimestepSystem::update(float time, float delta_time)
  {
    _acc += delta_time;
    while (_acc >= _dt) {
      updateSystem(time, _dt);
      _acc -= _dt;
    }
  }
}