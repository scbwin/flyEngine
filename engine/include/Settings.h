#ifndef SETTINGS_H
#define SETTINGS_H

#include <math/FlyMath.h>
#include <vector>

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
    unsigned _shadowMapSize = 1024;
    bool _shadows = true;
    bool _shadowPercentageCloserFiltering = true;
    float _smBias = 0.0035f;
    std::vector<float> _smFrustumSplits = { 30.f };
  //  std::vector<float> _smFrustumSplits = { 7.5f, 50.f, 300.f };
 //   std::vector<float> _smFrustumSplits = { 30.f, 50.f, 200.f };
    bool _normalMapping = true;
    bool _parallaxMapping = true;
    bool _steepParallax = true;
    unsigned _anisotropy = 4u;
  };
}

#endif
