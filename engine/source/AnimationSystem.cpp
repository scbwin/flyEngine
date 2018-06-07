#include "AnimationSystem.h"
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
  void AnimationSystem::update()
  {
    std::vector<std::shared_ptr<Animation>> to_delete;
    for (const auto& a : _animations) {
      float progress;
      if (_gameTimer->getTimeSeconds() >= a->getTimeEnd()) {
        progress = 1.f;
        to_delete.push_back(a);
      }
      else {
        progress = a->getInterpolator()->getInterpolation((_gameTimer->getTimeSeconds() - a->getTimeStart()) / (a->getTimeEnd() - a->getTimeStart()));
      }
      a->getUpdateFunction()(progress);
    }
    for (auto& a : to_delete) {
      a->getOnDelete()();
      _animations.erase(a);
    }
  }
  void AnimationSystem::startAnimation(const std::shared_ptr<Animation>& animation)
  {
    _animations.insert(animation);
  }
}