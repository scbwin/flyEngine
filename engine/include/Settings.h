#ifndef SETTINGS_H
#define SETTINGS_H

#include <math/FlyMath.h>

namespace fly
{
  struct DetailCullingParams
  {
    float _errorThreshold;
    float _errorExponent;
  };

  enum class DisplayListSortMode
  {
    MATERIAL,
    SHADER_AND_MATERIAL
  };

  struct Settings
  {
    bool _lensflareEnabled;
    bool _depthOfFieldEnabled;
    bool _motionBlurEnabled;
    bool _vsync;
    bool _ssrEnabled;
    bool _lightVolumesEnabled;
    bool _wireframe;
    Vec3f _depthOfFieldDistances;
    Vec3f _skyColor;
    float _brightScale;
    float _brightBias;
    float _exposure;
    unsigned _lensflareLevels;
    int _ssrSteps;
    float _ssrRayLenScale;
    float _ssrMinRayLen;
    int _smDepthBias;
    float _smSlopeScaledDepthBias;
    DetailCullingParams _detailCullingParams;
    DisplayListSortMode _dlSortMode = DisplayListSortMode::SHADER_AND_MATERIAL;
    bool _debugQuadtreeNodeAABBs = false;
    bool _debugObjectAABBs = false;
    bool _postProcessing = true;
  };
}

#endif
