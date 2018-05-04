#ifndef ANIMATIONSYSTEM_H
#define ANIMATIONSYSTEM_H

#include <memory>
#include <set>
#include "System.h"

namespace fly
{
  class Entity;

  class AnimationSystem : public System
  {
  public:
    AnimationSystem();
    virtual ~AnimationSystem();
    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void update(float time, float delta_time) override;
  private:
    std::set<Entity*> _entities;
  };
}

#endif
