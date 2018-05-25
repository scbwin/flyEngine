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
    _em.addListener(system);
  }
  void Engine::update()
  {
    _gameTimer.tick();
    for (const auto& s : _systems) {
      s->update();
    }
  }
  EntityManager* Engine::getEntityManager()
  {
    return &_em;
  }
  GameTimer * Engine::getGameTimer()
  {
    return &_gameTimer;
  }
}