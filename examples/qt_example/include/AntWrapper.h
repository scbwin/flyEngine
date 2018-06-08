#ifndef ANTWRAPPER_H
#define ANTWRAPPER_H

#include <AntTweakBar.h>
#include <memory>

namespace fly
{
  class GraphicsSettings;
  class OpenGLAPI;
  class Camera;
  class CameraController;
  class Entity;
  class GameTimer;
  class DirectionalLight;
  template<typename API, typename BV>
  class SkydomeRenderable;
  class AABB;
  template<typename API, typename BV>
  class Renderer;
}

class QWidget;

class AntWrapper
{
public:
  AntWrapper() = default;
  AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api, fly::CameraController* camera_controller, 
    std::shared_ptr<fly::SkydomeRenderable<fly::OpenGLAPI, fly::AABB>> skydome, fly::GameTimer* game_timer, QWidget* widget, 
    std::shared_ptr<fly::Camera> camera, fly::DirectionalLight* dl, fly::Renderer<fly::OpenGLAPI, fly::AABB>* renderer);
private:
  struct SkydomeData
  {
    fly::Renderer<fly::OpenGLAPI, fly::AABB>* _renderer;
    std::shared_ptr<fly::SkydomeRenderable<fly::OpenGLAPI, fly::AABB>> _skydome;
  };
  SkydomeData _skydomeData;
  static void setShadows(const void* value, void* client_data);
  static void getShadows(void* value, void* client_data);
  static void setPCF(const void* value, void* client_data);
  static void getPCF(void* value, void* client_data);
  static void setLightDirX(const void* value, void* client_data);
  static void getLightDirX(void* value, void* client_data);
  static void setLightDirY(const void* value, void* client_data);
  static void getLightDirY(void* value, void* client_data);
  static void setLightDirZ(const void* value, void* client_data);
  static void getLightDirZ(void* value, void* client_data);
  static void setGodRays(const void* value, void* client_data);
  static void getGodRays(void* value, void* client_data);
  static void setGodRayDecay(const void* value, void* client_data);
  static void getGodRayDecay(void* value, void* client_data);
  static void setGodRaySteps(const void* value, void* client_data);
  static void getGodRaySteps(void* value, void* client_data);
  static void setGodRayFadeDist(const void* value, void* client_data);
  static void getGodRayFadeDist(void* value, void* client_data);
  static void setMaxShadowCastDistance(const void* value, void* client_data);
  static void getMaxShadowCastDistance(void* value, void* client_data);
  static void setNormalMapping(const void* value, void* client_data);
  static void getNormalMapping(void* value, void* client_data);
  static void setParallaxMapping(const void* value, void* client_data);
  static void getParallaxMapping(void* value, void* client_data);
  static void setReliefMapping(const void* value, void* client_data);
  static void getReliefMapping(void* value, void* client_data);
  static void setDepthPrepass(const void* value, void* client_data);
  static void getDepthPrepass(void* value, void* client_data);
  static void setShadowmapsize(const void* value, void* client_data);
  static void getShadowmapsize(void* value, void* client_data);
  static void setExposure(const void* value, void* client_data);
  static void getExposure(void* value, void* client_data);
  static void setGammaEnabled(const void* value, void* client_data);
  static void getGammaEnabled(void* value, void* client_data);
  static void setGamma(const void* value, void* client_data);
  static void getGamma(void* value, void* client_data);
  static void setDebugBVH(const void* value, void* client_data);
  static void getDebugBVH(void* value, void* client_data);
  static void setDebugAABBs(const void* value, void* client_data);
  static void getDebugAABBs(void* value, void* client_data);
  static void setDetailCullingThreshold(const void* value, void* client_data);
  static void getDetailCullingThreshold(void* value, void* client_data);
  static void setCamSpeed(const void* value, void* client_data);
  static void getCamSpeed(void* value, void* client_data);
  static void setSkydome(const void* value, void* client_data);
  static void getSkydome(void* value, void* client_data);
  static void setShadowFactor(const void* value, void* client_data);
  static void getShadowFactor(void* value, void* client_data);
  static void setDepthOfField(const void* value, void* client_data);
  static void getDepthOfField(void* value, void* client_data);
  static void setDofNear(const void* value, void* client_data);
  static void getDofNear(void* value, void* client_data);
  static void setDofCenter(const void* value, void* client_data);
  static void getDofCenter(void* value, void* client_data);
  static void setDofFar(const void* value, void* client_data);
  static void getDofFar(void* value, void* client_data);
  static void setBlurRadius(const void* value, void* client_data);
  static void getBlurRadius(void* value, void* client_data);
  static void setBlurSigma(const void* value, void* client_data);
  static void getBlurSigma(void* value, void* client_data);
  static void setDofScaleFactor(const void* value, void* client_data);
  static void getDofScaleFactor(void* value, void* client_data);
  static void setGamePaused(const void* value, void* client_data);
  static void getGamePaused(void* value, void* client_data);
  static void setSSR(const void* value, void* client_data);
  static void getSSR(void* value, void* client_data);
  static void setSSRSteps(const void* value, void* client_data);
  static void getSSRSteps(void* value, void* client_data);
  static void setSSRBinarySteps(const void* value, void* client_data);
  static void getSSRBinarySteps(void* value, void* client_data);
  static void setSSRRayLenScale(const void* value, void* client_data);
  static void getSSRRayLenScale(void* value, void* client_data);
  static void setSSRMinRayLen(const void* value, void* client_data);
  static void getSSRMinRayLen(void* value, void* client_data);
  static void setSSRBlendWeight(const void* value, void* client_data);
  static void getSSRBlendWeight(void* value, void* client_data);
  static void setFullScreen(const void* value, void* client_data);
  static void getFullScreen(void* value, void* client_data);
  static void setSMPOFactor(const void* value, void* client_data);
  static void setSMPOUnits(const void* value, void* client_data);
  static void getSMPOFactor(void* value, void* client_data);
  static void getSMPOUnits(void* value, void* client_data);
  static void setMTCulling(const void* value, void* client_data);
  static void getMTCulling(void* value, void* client_data);
  template<typename T> static T* cast(void* data){return reinterpret_cast<T*>(data);}
  template<typename T> static const T* cast(const void* data) { return reinterpret_cast<const T*>(data); }
};

#endif // !ANTWRAPPER_H
