#ifndef RENDERPARAMS_H
#define RENDERPARAMS_H

#include <math/FlyMath.h>
#include <vector>
#include <WindParams.h>
#include <StackPOD.h>

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
    Mat3f _viewMatrixInverse;
    Mat4f const * _VP;
    StackPOD<Mat4f> _worldToLight;
    std::vector<float> const * _smFrustumSplits;
    float _smBias;
    float _shadowDarkenFactor;
    Vec3f const * _lightPosWorld;
    Vec3f _camPosworld;
    Vec3f const * _lightIntensity;
    ZNearMapping _zNearMapping;
    float _time;
    WindParams _windParams;
    float _exposure;
    float _gamma;
  };
}

#endif