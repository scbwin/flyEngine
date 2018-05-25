#include <physics/PhysicsSystem.h>
#include <Entity.h>
#include <GameTimer.h>

namespace fly
{
  void PhysicsSystem::onComponentAdded(Entity * entity, const std::shared_ptr<Component>& component)
  {
    if (entity->getComponent<ParticleSystem>() == component) {
      _particleSystems[entity] = entity->getComponent<ParticleSystem>();
    }
  }
  void PhysicsSystem::onComponentRemoved(Entity * entity, const std::shared_ptr<Component>& component)
  {
    if (entity->getComponent<ParticleSystem>() == component) {
      _particleSystems.erase(entity);
    }
  }
  void PhysicsSystem::updateSystem()
  {
    for (auto& ps : _particleSystems) {
      ps.second->update(_gameTimer->getTimeSeconds(), _gameTimer->getDeltaTimeSeconds());
    }
    /*  float max_delta = 1.f / 300.f;
      float delta = delta_time;
      while (delta > 0.f) {
        float delta_new = std::min(delta, max_delta);
        for (auto& ps : _particleSystems) {
          ps.second->update(time, delta_new);
        }
        delta -= delta_new;
      }*/
  }
}