#include "Animation.h"
#include <iostream>

namespace fly
{
  Animation::Animation(float duration_seconds, float current_time, const std::function<void(float)>& update_function, const std::shared_ptr<Interpolator>& interpolator) :
    _duration(duration_seconds), 
    _updateFunction(update_function),
    _timeStart(current_time),
    _timeEnd(current_time + duration_seconds),
    _interpolator(interpolator)
  {
  }
  float Animation::getDuration()
  {
    return _duration;
  }
  float Animation::getTimeStart()
  {
    return _timeStart;
  }
  float Animation::getTimeEnd()
  {
    return _timeEnd;
  }
  std::function<void(float)>& Animation::getUpdateFunction()
  {
    return _updateFunction;
  }
  std::shared_ptr<Animation::Interpolator> Animation::getInterpolator()
  {
    return _interpolator;
  }
  float Animation::LinearInterpolator::getInterpolation(float t)
  {
    return t;
  }
  float Animation::EaseInEaseOutInterpolator::getInterpolation(float t)
  {
    return t * t * (3.f - 2.f * t);
  }
  Animation::OvershootInterpolator::OvershootInterpolator(float tension) : _tension(tension)
  {
  }
  float Animation::OvershootInterpolator::getInterpolation(float t)
  {
    t -= 1.f;
    return t * t *((_tension + 1.f) * t + _tension) + 1.f;
  }
}