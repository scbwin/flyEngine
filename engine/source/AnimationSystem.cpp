#include "AnimationSystem.h"
#include "Entity.h"
#include "Animation.h"
#include "EntityManager.h"
#include <vector>

namespace fly
{
  AnimationSystem::AnimationSystem()
  {
  }
  AnimationSystem::~AnimationSystem()
  {
  }
  void AnimationSystem::onComponentsChanged(Entity* entity)
  {
    if (entity->getComponent<Animation>()) {
      _entities.insert(entity);
    }
    else {
      _entities.erase(entity);
    }
  }
  void AnimationSystem::update(float time, float delta_time)
  {
    std::vector<Entity*> to_delete;
    for (auto& e : _entities) {
      auto a = e->getComponent<Animation>();
      float progress;
      if (time >= a->getTimeEnd()) {
        progress = 1.f;
        to_delete.push_back(e);
      }
      else {
        progress = a->getInterpolator()->getInterpolation((time - a->getTimeStart()) / (a->getTimeEnd() - a->getTimeStart()));
      }
      a->getUpdateFunction()(progress);
    }
    for (auto& e : to_delete) {
      e->removeComponent<Animation>();
    }
  }
}