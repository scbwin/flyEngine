#include "AnimationSystem.h"
#include "Entity.h"
#include "Animation.h"
#include <vector>
#include <iostream>
#include <GameTimer.h>

namespace fly
{
  AnimationSystem::AnimationSystem()
  {
  }
  AnimationSystem::~AnimationSystem()
  {
  }
  void AnimationSystem::onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component)
  {
    addIfInterested(entity, component, _animations);
  }
  void AnimationSystem::onComponentRemoved(Entity * entity, const std::shared_ptr<Component>& component)
  {
    deleteIfInterested(entity, component, _animations);
  }
  void AnimationSystem::update()
  {
    std::vector<Entity*> to_delete;
    for (const auto& e : _animations) {
      const auto& a = e.second;
      float progress;
      if (_gameTimer->getTimeSeconds() >= a->getTimeEnd()) {
        progress = 1.f;
        to_delete.push_back(e.first);
      }
      else {
        progress = a->getInterpolator()->getInterpolation((_gameTimer->getTimeSeconds() - a->getTimeStart()) / (a->getTimeEnd() - a->getTimeStart()));
      }
      a->getUpdateFunction()(progress);
    }
    for (auto& e : to_delete) {
      e->getComponent<Animation>()->getOnDelete()();
    }
  }
}