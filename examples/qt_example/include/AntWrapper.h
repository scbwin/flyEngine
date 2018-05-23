#ifndef ANTWRAPPER_H
#define ANTWRAPPER_H

#include <AntTweakBar.h>

namespace fly
{
  class GraphicsSettings;
  class OpenGLAPI;
  class Camera;
  class CameraController;
  class Entity;
  class GameTimer;
  class DirectionalLight;
}

class QWidget;

class AntWrapper
{
public:
  AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api, fly::Camera* camera, 
    fly::CameraController* camera_controller, fly::Entity* skydome, fly::GameTimer* game_timer, QWidget* widget, fly::DirectionalLight* dl);
private:
  static void cbSetShadows(const void* value, void* client_data);
  static void cbGetShadows(void* value, void* client_data);
  static void cbSetPCF(const void* value, void* client_data);
  static void cbGetPCF(void* value, void* client_data);
  static void cbSetNormalMapping(const void* value, void* client_data);
  static void cbGetNormalMapping(void* value, void* client_data);
  static void cbSetParallaxMapping(const void* value, void* client_data);
  static void cbGetParallaxMapping(void* value, void* client_data);
  static void cbSetReliefMapping(const void* value, void* client_data);
  static void cbGetReliefMapping(void* value, void* client_data);
  static void cbSetDepthPrepass(const void* value, void* client_data);
  static void cbGetDepthPrepass(void* value, void* client_data);
  static void cbSetWindAnimations(const void* value, void* client_data);
  static void cbGetWindAnimations(void* value, void* client_data);
  static void cbSetShadowmapsize(const void* value, void* client_data);
  static void cbGetShadowmapsize(void* value, void* client_data);
  static void cbSetExposure(const void* value, void* client_data);
  static void cbGetExposure(void* value, void* client_data);
  static void cbSetGammaEnabled(const void* value, void* client_data);
  static void cbGetGammaEnabled(void* value, void* client_data);
  static void cbSetGamma(const void* value, void* client_data);
  static void cbGetGamma(void* value, void* client_data);
  static void cbSetDebugQuadtree(const void* value, void* client_data);
  static void cbGetDebugQuadtree(void* value, void* client_data);
  static void cbSetDebugAABBs(const void* value, void* client_data);
  static void cbGetDebugAABBs(void* value, void* client_data);
  static void cbReloadShaders(void* client_data);
  static void setCameraLerping(const void* value, void* client_data);
  static void getCameraLerping(void* value, void* client_data);
  static void setCameraLerpAmount(const void* value, void* client_data);
  static void getCameraLerpAmount(void* value, void* client_data);
  static void setDetailCulling(const void* value, void* client_data);
  static void getDetailCulling(void* value, void* client_data);
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
  static void setDLEulerX(const void* value, void* client_data);
  static void getDLEulerX(void* value, void* client_data);
  static void setDLEulerY(const void* value, void* client_data);
  static void getDLEulerY(void* value, void* client_data);
  static void setDLEulerZ(const void* value, void* client_data);
  static void getDLEulerZ(void* value, void* client_data);

  template<typename T> static T* cast(void* data){return reinterpret_cast<T*>(data);}
  template<typename T> static const T* cast(const void* data) { return reinterpret_cast<const T*>(data); }
};

#endif // !ANTWRAPPER_H
