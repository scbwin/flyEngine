#include <GraphicsSettings.h>
#include <algorithm>

namespace fly
{
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
    notifyShadowsChanged();
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
      l->shadowMapSizeChanged(getShadowMapSize());
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
  }
  bool GraphicsSettings::getWindAnimations() const
  {
    return _windAnimations;
  }
  void GraphicsSettings::setWindAnimations(bool enabled)
  {
    _windAnimations = enabled;
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->windAnimationsChanged(getWindAnimations());
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
      l->anisotropyChanged(getAnisotropy());
    });
  }
  void GraphicsSettings::setExposureEnabled(bool exposure)
  {
    _exposureEnabled = exposure;
    notifyCompositingChanged();
  }
  void GraphicsSettings::notifiyNormalMappingChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->normalMappingChanged(getNormalMapping(), getParallaxMapping(), getReliefMapping());
    });
  }
  void GraphicsSettings::notifyShadowsChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->shadowsChanged(getShadows(), getShadowsPCF(), getShadowBias(), getFrustumSplits());
    });
  }
  void GraphicsSettings::notifyCompositingChanged()
  {
    notifiyListeners([this](const std::shared_ptr<Listener>& l) {
      l->compositingChanged(exposureEnabled(), depthPrepassEnabled(), postProcessingEnabled());
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
}