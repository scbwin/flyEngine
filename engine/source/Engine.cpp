#include <Engine.h>
#include <System.h>

namespace fly
{
  Engine::Engine()
  {
  }
  void Engine::addSystem(const std::shared_ptr<System>& system)
  {
    system->setGameTimer(&_gameTimer);
    _systems.push_back(system);
  }
  void Engine::update()
  {
    _gameTimer.tick();
    for (const auto& s : _systems) {
      s->update();
    }
  }
  GameTimer* Engine::getGameTimer()
  {
    return &_gameTimer;
  }
}