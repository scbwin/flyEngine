#ifndef GRAPHICSSETTINGS_H
#define GRAPHICSSETTINGS_H

#include <set>
#include <memory>
#include <vector>
#include <functional>

namespace fly
{
  class GraphicsSettings
  {
  public:
    class Listener
    {
    public:
      virtual ~Listener() = default;
      virtual void normalMappingChanged(GraphicsSettings const * const gs) = 0;
      virtual void shadowsChanged(GraphicsSettings const * const gs) = 0;
      virtual void shadowMapSizeChanged(GraphicsSettings const * const gs) = 0;
      virtual void compositingChanged(GraphicsSettings const * const gs) = 0;
      virtual void windAnimationsChanged(GraphicsSettings const * const gs) = 0;
      virtual void anisotropyChanged(GraphicsSettings const * const gs) = 0;
      virtual void cameraLerpingChanged(GraphicsSettings const * const gs) = 0;
    };
    void addListener(const std::shared_ptr<Listener>& listener);
    void setNormalMapping(bool normal_mapping);
    void setParallaxMapping(bool parallax_mapping);
    void setReliefMapping(bool relief_mapping);
    bool getNormalMapping() const;
    bool getParallaxMapping() const;
    bool getReliefMapping() const;
    void setShadows(bool shadows);
    void setShadowsPCF(bool pcf);
    void setShadowBias(float bias);
    void setFrustumSplits(const std::vector<float>& frustum_splits);
    void setShadowMapSize(unsigned size);
    bool getShadows() const;
    bool getShadowsPCF() const;
    float getShadowBias() const;
    const std::vector<float>& getFrustumSplits() const;
    unsigned getShadowMapSize() const;
    bool exposureEnabled() const;
    bool postProcessingEnabled() const;
    bool depthPrepassEnabled() const;
    void setExposureEnabled(bool enabled);
    void setPostProcessingEnabled(bool enabled);
    void setDepthprepassEnabled(bool enabled);
    float getExposure() const;
    void setExposure(float exposure);
    bool getWindAnimations() const;
    void setWindAnimations(bool enabled);
    bool getDebugQuadtreeNodeAABBs() const;
    bool getDebugObjectAABBs() const;
    void setDebugQuadtreeNodeAABBs(bool enable);
    void setDebugObjectAABBs(bool enable);
    unsigned getAnisotropy() const;
    void setAnisotropy(unsigned anisotropy);
    bool getCameraLerping() const;
    float getCameraLerpAlpha() const;
    void setCameraLerping(bool enable);
    void setCameraLerping(float alpha);
    bool getDetailCulling() const;
    void setDetailCulling(bool enabled);

  private:
    std::set<std::weak_ptr<Listener>, std::owner_less<std::weak_ptr<Listener>>> _listeners;
    bool _normalMapping = true;
    bool _parallaxMapping = true;
    bool _reliefMapping = true;
    bool _exposureEnabled = true;
    float _exposure = 1.f;
    bool _windAnimations = true;
    bool _depthPrepass = false;
    bool _shadows = true;
    bool _shadowsPCF = true;
    float _smBias = 0.0035f;
    unsigned _shadowMapSize = 1024;
    std::vector<float> _smFrustumSplits = { 7.5f, 50.f, 500.f };
    unsigned _anisotropy = 4u;
    bool _postProcessing = true;
    bool _debugQuadtreeNodeAABBs = false;
    bool _debugObjectAABBs = false;
    bool _cameraLerping = true;
    float _cameraLerpAlpha = 0.8f;
    bool _detailCulling = true;

    void notifiyNormalMappingChanged();
    void notifyShadowsChanged();
    void notifyCompositingChanged();
    void notifiyListeners(const std::function<void(const std::shared_ptr<Listener>&)>& notify_func);
  };
}

#endif
