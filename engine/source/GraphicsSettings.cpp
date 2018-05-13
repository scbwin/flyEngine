#include <GraphicsSettings.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>

namespace fly
{
  GraphicsSettings::GraphicsSettings()
  {
    computeBlurWeights();
  }
  void GraphicsSettings::addListener(const std::shared_ptr<Listener>& listener)
  {
    _listeners.insert(listener);
  }
  void GraphicsSettings::setNormalMapping(bool normal_mapping)
  {
    _normalMapping = normal_mapping || _parallaxMapping; // Don't disable if parallax is enabled
    notifiyNormalMappingChanged();
  }
  void GraphicsSettings::setParallaxMapping(bool parallax_mapping)
  {
    _parallaxMapping = (parallax_mapping && _normalMapping) || _reliefMapping; // Only enable if normal enabled, don't disable if relief is enabled
    notifiyNormalMappingChanged();
  }
  void GraphicsSettings::setReliefMapping(bool relief_mapping)
  {
    _reliefMapping = relief_mapping && _normalMapping && _parallaxMapping; // Only enable if normal and parallax enabled
    notifiyNormalMappingChanged();
  }
  bool GraphicsSettings::getNormalMapping() const
  {
    return _normalMapping;
  }
  bool GraphicsSettings::getParallaxMapping() const
  {
    return _parallaxMapping;
  }
  bool GraphicsSettings::getReliefMapping() const
  {
    return _reliefMapping;
  }
  void GraphicsSettings::setShadows(bool shadows)
  {
    _shadows = shadows || _shadowsPCF;
    notifyShadowsChanged();
  }
  void GraphicsSettings::setShadowsPCF(bool pcf)
  {
    _shadowsPCF = pcf && _shadows;
    notifyShadowsChanged();
  }
  void GraphicsSettings::setShadowBias(float bias)
  {
    _smBias = bias;
  }
  void GraphicsSettings::setFrustumSplits(const std::vector<float>& frustum_splits)
  {
    _smFrustumSplits = frustum_splits;
    notifyShadowsChanged();
  }
  void GraphicsSettings::setShadowMapSize(unsigned size)
  {
    _shadowMapSize = std::max(1u, size);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->shadowMapSizeChanged(this);
    });
  }
  bool GraphicsSettings::getShadows() const
  {
    return _shadows;
  }
  bool GraphicsSettings::getShadowsPCF() const
  {
    return _shadowsPCF;
  }
  float GraphicsSettings::getShadowBias() const
  {
    return _smBias;
  }
  const std::vector<float>& GraphicsSettings::getFrustumSplits() const
  {
    return _smFrustumSplits;
  }
  unsigned GraphicsSettings::getShadowMapSize() const
  {
    return _shadowMapSize;
  }
  bool GraphicsSettings::exposureEnabled() const
  {
    return _exposureEnabled;
  }
  bool GraphicsSettings::gammaEnabled() const
  {
    return _gammaCorrectionEnabled;
  }
  bool GraphicsSettings::postProcessingEnabled() const
  {
    return _postProcessing;
  }
  bool GraphicsSettings::depthPrepassEnabled() const
  {
    return _depthPrepass;
  }
  void GraphicsSettings::setPostProcessingEnabled(bool enabled)
  {
    _postProcessing = enabled;
    notifyCompositingChanged();
  }
  void GraphicsSettings::setDepthprepassEnabled(bool enabled)
  {
    _depthPrepass = enabled;
    notifyCompositingChanged();
  }
  float GraphicsSettings::getExposure() const
  {
    return _exposure;
  }
  void GraphicsSettings::setExposure(float exposure)
  {
    _exposure = exposure;
    notifyCompositingChanged();
  }
  float GraphicsSettings::getGamma() const
  {
    return _gamma;
  }
  void GraphicsSettings::setGamma(float gamma)
  {
    _gamma = std::max(gamma, 0.f);
    notifyCompositingChanged();
  }
  bool GraphicsSettings::getWindAnimations() const
  {
    return _windAnimations;
  }
  void GraphicsSettings::setWindAnimations(bool enabled)
  {
    _windAnimations = enabled;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->windAnimationsChanged(this);
    });
  }
  bool GraphicsSettings::getDebugQuadtreeNodeAABBs() const
  {
    return _debugQuadtreeNodeAABBs;
  }
  bool GraphicsSettings::getDebugObjectAABBs() const
  {
    return _debugObjectAABBs;
  }
  void GraphicsSettings::setDebugQuadtreeNodeAABBs(bool enable)
  {
    _debugQuadtreeNodeAABBs = enable;
  }
  void GraphicsSettings::setDebugObjectAABBs(bool enable)
  {
    _debugObjectAABBs = enable;
  }
  unsigned GraphicsSettings::getAnisotropy() const
  {
    return _anisotropy;
  }
  void GraphicsSettings::setAnisotropy(unsigned anisotropy)
  {
    _anisotropy = anisotropy;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->anisotropyChanged(this);
    });
  }
  bool GraphicsSettings::getCameraLerping() const
  {
    return _cameraLerping;
  }
  float GraphicsSettings::getCameraLerpAlpha() const
  {
    return _cameraLerpAlpha;
  }
  void GraphicsSettings::setCameraLerping(float alpha)
  {
    _cameraLerpAlpha = glm::clamp(alpha, 0.f, 0.99f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->cameraLerpingChanged(this);
    });
  }
  bool GraphicsSettings::getDetailCulling() const
  {
    return _detailCulling;
  }
  void GraphicsSettings::setDetailCulling(bool enabled)
  {
    _detailCulling = enabled;
  }
  void GraphicsSettings::setShadowDarkenFactor(float factor)
  {
    _shadowDarkenFactor = glm::clamp(factor, 0.f, 1.f);
  }
  float GraphicsSettings::getShadowDarkenFactor() const
  {
    return _shadowDarkenFactor;
  }
  bool GraphicsSettings::getDepthOfField() const
  {
    return _depthOfField;
  }
  void GraphicsSettings::setDepthOfField(bool enabled)
  {
    _depthOfField = enabled && _postProcessing;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  float GraphicsSettings::getBlurSigma() const
  {
    return _blurSigma;
  }
  void GraphicsSettings::setBlurSigma(float sigma)
  {
    _blurSigma = sigma;
    computeBlurWeights();
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  unsigned GraphicsSettings::getBlurRadius() const
  {
    return _blurRadius;
  }
  void GraphicsSettings::setBlurRadius(unsigned radius)
  {
    _blurRadius = std::max(radius, 1u);
    computeBlurWeights();
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  float GraphicsSettings::getDepthOfFieldScaleFactor() const
  {
    return _depthOfFieldScaleFactor;
  }
  void GraphicsSettings::setDepthOfFieldScaleFactor(float scale_factor)
  {
    _depthOfFieldScaleFactor = glm::clamp(scale_factor, 0.01f, 1.f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  const std::vector<float>& GraphicsSettings::getBlurWeights() const
  {
    return _blurWeights;
  }
  float GraphicsSettings::getDofNear() const
  {
    return _dofNear;
  }
  float GraphicsSettings::getDofCenter() const
  {
    return _dofCenter;
  }
  float GraphicsSettings::getDofFar() const
  {
    return _dofFar;
  }
  void GraphicsSettings::setDofNear(float near)
  {
    _dofNear = std::min(near, _dofCenter);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  void GraphicsSettings::setDofCenter(float center)
  {
    _dofCenter = glm::clamp(center, _dofNear, _dofFar);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  void GraphicsSettings::setDofFar(float far)
  {
    _dofFar = std::max(far, _dofCenter);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->depthOfFieldChanged(this);
    });
  }
  bool GraphicsSettings::getScreenSpaceReflections() const
  {
    return _screenSpaceReflections;
  }
  void GraphicsSettings::setScreenSpaceReflections(bool enabled)
  {
    _screenSpaceReflections = enabled && _postProcessing;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  float GraphicsSettings::getSSRSteps() const
  {
    return _ssrSteps;
  }
  unsigned GraphicsSettings::getSSRBinarySteps() const
  {
    return _ssrBinarySteps;
  }
  float GraphicsSettings::getSSRRayLenScale() const
  {
    return _ssrRayLenScale;
  }
  float GraphicsSettings::getSSRMinRayLen() const
  {
    return _ssrMinRayLen;
  }
  float GraphicsSettings::getSSRBlendWeight() const
  {
    return _ssrBlendWeight;
  }
  void GraphicsSettings::setSSRSteps(float steps)
  {
    _ssrSteps = std::max(steps, 0.f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  void GraphicsSettings::setSSRBinarySteps(unsigned steps)
  {
    _ssrBinarySteps = steps;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  void GraphicsSettings::setSSRRayLenScale(float scale)
  {
    _ssrRayLenScale = std::max(scale, 0.f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  void GraphicsSettings::setSSRMinRayLen(float len)
  {
    _ssrMinRayLen = std::max(len, 0.f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  void GraphicsSettings::setSSRBlendWeight(float weight)
  {
    _ssrBlendWeight = glm::clamp(weight, 0.f, 1.f);
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->screenSpaceReflectionsChanged(this);
    });
  }
  void GraphicsSettings::setCameraLerping(bool enable)
  {
    _cameraLerping = enable;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->cameraLerpingChanged(this);
    });
  }
  void GraphicsSettings::setExposureEnabled(bool exposure)
  {
    _exposureEnabled = exposure;
    notifyCompositingChanged();
  }
  void GraphicsSettings::setGammaCorrectionEnabled(bool enabled)
  {
    _gammaCorrectionEnabled = enabled;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->gammaChanged(this);
    });
  }
  void GraphicsSettings::notifiyNormalMappingChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->normalMappingChanged(this);
    });
  }
  void GraphicsSettings::notifyShadowsChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->shadowsChanged(this);
    });
  }
  void GraphicsSettings::notifyCompositingChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->compositingChanged(this);
    });
  }
  void GraphicsSettings::notifiyListeners(const std::function<void(const std::shared_ptr<Listener>&)>& notify_func)
  {
    if (_listeners.size()) {
      std::vector<std::weak_ptr<Listener>> delete_list;
      for (const auto& ptr : _listeners) {
        auto l = ptr.lock();
        l ? notify_func(l) : delete_list.push_back(ptr);
      }
      for (const auto& ptr : delete_list) {
        _listeners.erase(ptr);
      }
    }
  }
  void GraphicsSettings::computeBlurWeights()
  {
    _blurWeights.clear();
    float sum = 0.f;
    for (int i = -static_cast<int>(_blurRadius); i <= static_cast<int>(_blurRadius); i++) {
      _blurWeights.push_back(exp(-(i * i) / (2 * _blurSigma * _blurSigma)));
      sum += _blurWeights.back();
    }
    for (auto& w : _blurWeights) {
      w /= sum;
    }
  }
}