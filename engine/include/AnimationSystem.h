#ifndef ANIMATIONSYSTEM_H
#define ANIMATIONSYSTEM_H

#include <memory>
#include <set>
#include "System.h"

namespace fly
{
  class Entity;
  class Animation;

  class AnimationSystem : public System
  {
  public:
    AnimationSystem();
    virtual ~AnimationSystem();
    virtual void update() override;
    void startAnimation(const std::shared_ptr<Animation>& animation);
  private:
    std::set<std::shared_ptr<Animation>> _animations;
  };
}

#endif
