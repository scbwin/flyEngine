#ifndef ENGINE_H
#define ENGINE_H

#include <Leakcheck.h>
#include <memory>
#include <vector>
#include <EntityManager.h>
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
    EntityManager* getEntityManager();
    GameTimer * getGameTimer();
  private:
    EntityManager _em;
    std::vector<std::shared_ptr<System>> _systems;
    GameTimer _gameTimer;
  };
}

#endif
