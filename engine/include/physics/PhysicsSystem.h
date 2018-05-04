#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include <FixedTimestepSystem.h>
#include <map>
#include <physics/ParticleSystem.h>

namespace fly
{
  class PhysicsSystem : public FixedTimestepSystem
  {
  public:
    PhysicsSystem() = default;
    virtual ~PhysicsSystem() = default;

    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void updateSystem(float time, float delta_time) override;

  private:
    std::map<Entity*, std::shared_ptr<ParticleSystem>> _particleSystems;
  };
}

#endif // !PHYSICSSYSTEM_H
