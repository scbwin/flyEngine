#ifndef RENDERPARAMS_H
#define RENDERPARAMS_H

#include <math/FlyMath.h>

namespace fly
{
  enum class ZNearMapping
  {
    ZERO, // directx
    MINUS_ONE // opengl
  };

  struct GlobalShaderParams
  {
    Mat4f _projectionMatrix;
    Mat4f _viewMatrix;
    Mat4f _VP;
    Mat4f _Vinverse;
    Mat4f _viewToLight;
    Vec3f _lightPosView;
    Vec3f _lightIntensity;
    ZNearMapping _zNearMapping;
  };
}

#endif