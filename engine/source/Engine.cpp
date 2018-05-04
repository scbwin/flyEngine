#include <Engine.h>
#include <System.h>

namespace fly
{
  Engine::Engine()
  {
  }
  void Engine::addSystem(const std::shared_ptr<System>& system)
  {
    _systems.insert(system);
    _em.addListener(system);
  }
  void Engine::update(float time, float delta_time)
  {
    for (auto& s : _systems) {
      s->update(time, delta_time);
    }
  }
  EntityManager* Engine::getEntityManager()
  {
    return &_em;
  }
}