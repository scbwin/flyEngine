#ifndef GRAPHICSSETTINGS_H
#define GRAPHICSSETTINGS_H

#include <list>
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
      virtual void normalMappingChanged(GraphicsSettings const * gs) = 0;
      virtual void shadowsChanged(GraphicsSettings const * gs) = 0;
      virtual void shadowMapSizeChanged(GraphicsSettings const * gs) = 0;
      virtual void depthOfFieldChanged(GraphicsSettings const * gs) = 0;
      virtual void compositingChanged(GraphicsSettings const * gs) = 0;
      virtual void windAnimationsChanged(GraphicsSettings const * gs) = 0;
      virtual void anisotropyChanged(GraphicsSettings const * gs) = 0;
      virtual void cameraLerpingChanged(GraphicsSettings const * gs) = 0;
      virtual void gammaChanged(GraphicsSettings const * gs) = 0;
      virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) = 0;
    };
    GraphicsSettings();
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
    bool gammaEnabled() const;
    bool postProcessingEnabled() const;
    bool depthPrepassEnabled() const;
    void setExposureEnabled(bool enabled);
    void setGammaCorrectionEnabled(bool enabled);
    void setPostProcessingEnabled(bool enabled);
    void setDepthprepassEnabled(bool enabled);
    float getExposure() const;
    void setExposure(float exposure);
    float getGamma() const;
    void setGamma(float gamma);
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
    void setShadowDarkenFactor(float factor);
    float getShadowDarkenFactor() const;
    bool getDepthOfField() const;
    void setDepthOfField(bool enabled);
    float getBlurSigma() const;
    void setBlurSigma(float sigma);
    unsigned getBlurRadius() const;
    void setBlurRadius(unsigned radius);
    float getDepthOfFieldScaleFactor() const;
    void setDepthOfFieldScaleFactor(float scale_factor);
    const std::vector<float>& getBlurWeights() const;
    float getDofNear() const;
    float getDofCenter() const;
    float getDofFar() const;
    void setDofNear(float near);
    void setDofCenter(float center);
    void setDofFar(float far);
    bool getScreenSpaceReflections() const;
    void setScreenSpaceReflections(bool enabled);
    float getSSRSteps() const;
    unsigned getSSRBinarySteps() const;
    float getSSRRayLenScale() const;
    float getSSRMinRayLen() const;
    float getSSRBlendWeight() const;
    void setSSRSteps(float steps);
    void setSSRBinarySteps(unsigned steps);
    void setSSRRayLenScale(float scale);
    void setSSRMinRayLen(float len);
    void setSSRBlendWeight(float weight);

  private:
    std::list<std::weak_ptr<Listener>> _listeners;
    bool _normalMapping = true;
    bool _parallaxMapping = true;
    bool _reliefMapping = true;
    bool _exposureEnabled = true;
    bool _gammaCorrectionEnabled = true;
    float _exposure = 1.f;
    float _gamma = 2.2f;
    bool _windAnimations = true;
    bool _depthPrepass = false;
    bool _shadows = true;
    bool _shadowsPCF = true;
    float _smBias = 0.0035f;
    float _shadowDarkenFactor = 0.8f;
    unsigned _shadowMapSize = 1024;
    std::vector<float> _smFrustumSplits = { 7.5f, 50.f, 500.f };
    unsigned _anisotropy = 4u;
    bool _postProcessing = true;
    bool _debugQuadtreeNodeAABBs = false;
    bool _debugObjectAABBs = false;
    bool _cameraLerping = true;
    float _cameraLerpAlpha = 0.8f;
    bool _detailCulling = true;
    float _blurSigma = 2.5f;
    unsigned _blurRadius = 3u;
    std::vector<float> _blurWeights;
    bool _depthOfField = true;
    float _depthOfFieldScaleFactor = 0.5f;
    float _dofNear = 1.f;
    float _dofCenter = 5.f;
    float _dofFar = 50.f;
    bool _screenSpaceReflections = true;
    float _ssrSteps = 16.f;
    unsigned _ssrBinarySteps = 10u;
    float _ssrRayLenScale = 3.f;
    float _ssrMinRayLen = 24.f;
    float _ssrBlendWeight = 0.8f;

    void notifiyNormalMappingChanged();
    void notifyShadowsChanged();
    void notifyCompositingChanged();
    void notifiyListeners(const std::function<void(const std::shared_ptr<Listener>&)>& notify_func);
    void computeBlurWeights();
  };
}

#endif
