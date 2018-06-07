#ifndef ENGINE_H
#define ENGINE_H

#include <Leakcheck.h>
#include <memory>
#include <vector>
#include <GameTimer.h>

namespace fly
{
  class System;

  class Engine
  {
  public:
    Engine();
    void addSystem(const std::shared_ptr<System>& system);
    void update();
    GameTimer * getGameTimer();
  private:
    std::vector<std::shared_ptr<System>> _systems;
    GameTimer _gameTimer;
  };
}

#endif
