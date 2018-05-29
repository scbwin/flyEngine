#ifndef GLOBALSHADERPARAMS_H
#define GLOBALSHADERPARAMS_H

#include <math/FlyMath.h>
#include <StackPOD.h>
#include <vector>
#include <WindParams.h>

namespace fly
{
  struct GlobalShaderParams
  {
    Mat4f _projectionMatrix;
    Mat4f _viewMatrix;
    Mat3f _viewMatrixInverse;
    Mat4f const * _VP;
    StackPOD<Mat4f> _worldToLight;
    std::vector<float> const * _smFrustumSplits;
    float _shadowDarkenFactor;
    Vec3f const * _lightPosWorld;
    Vec3f _camPosworld;
    Vec3f const * _lightIntensity;
    float _time;
    WindParams _windParams;
    float _exposure;
    float _gamma;
  };
}

#endif // !GLOBALSHADERPARAMS_H
