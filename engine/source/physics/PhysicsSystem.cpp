#include <physics/PhysicsSystem.h>
#include <Entity.h>

namespace fly
{
  void PhysicsSystem::onComponentsChanged(Entity * entity)
  {
    auto ps = entity->getComponent<ParticleSystem>();
    if (ps) {
      _particleSystems[entity] = ps;
    }
    else {
      _particleSystems.erase(entity);
    }
  }
  void PhysicsSystem::updateSystem(float time, float delta_time)
  {
    for (auto& ps : _particleSystems) {
      ps.second->update(time, delta_time);
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