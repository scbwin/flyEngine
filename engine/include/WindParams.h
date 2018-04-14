#ifndef WINDPARAMS_H
#define WINDPARAMS_H

#include <math/FlyMath.h>

namespace fly
{
  struct WindParams
  {
    Vec2f _dir = Vec2f(0.f, 1.f);
    Vec2f _movement = Vec2f(1.5f, 1.5f);
    float _frequency = 2.5f;
    float _strength = 0.3f;
  };
}

#endif
