#ifndef ANIMATION_H
#define ANIMATION_H

#include "Component.h"
#include <functional>
#include <memory>

namespace fly
{
  class Animation : public Component
  {
  public:

    class Interpolator
    {
    public:
      Interpolator() = default;
      virtual ~Interpolator() = default;
      virtual float getInterpolation(float t) = 0;
    };

    class LinearInterpolator : public Interpolator
    {
    public:
      LinearInterpolator() = default;
      virtual ~LinearInterpolator() = default;
      virtual float getInterpolation(float t) override;
    };

    class EaseInEaseOutInterpolator : public Interpolator
    {
    public:
      EaseInEaseOutInterpolator() = default;
      virtual ~EaseInEaseOutInterpolator() = default;
      virtual float getInterpolation(float t) override;
    };

    class OvershootInterpolator : public Interpolator
    {
    public:
      OvershootInterpolator(float tension = 1.f);
      virtual ~OvershootInterpolator() = default;
      virtual float getInterpolation(float t) override;
    private:
      float _tension;
    };

    Animation(float duration_seconds, float current_time, 
      const std::function<void(float)>& update_function, const std::shared_ptr<Interpolator>& interpolator = std::make_shared<LinearInterpolator>());
    float getDuration();
    float getTimeStart();
    float getTimeEnd();
    std::function<void(float)>& getUpdateFunction();
    std::shared_ptr<Interpolator> getInterpolator();
  private:
    float _duration;
    float _timeStart;
    float _timeEnd;
    std::function<void(float)> _updateFunction;
    std::shared_ptr<Interpolator> _interpolator;
  };
}

#endif
