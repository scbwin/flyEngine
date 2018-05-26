#include <AntWrapper.h>
#include <GraphicsSettings.h>
#include <opengl/OpenGLAPI.h>
#include <Camera.h>
#include <CameraController.h>
#include <Entity.h>
#include <SkydomeRenderable.h>
#include <Model.h>
#include <GameTimer.h>
#include <qwidget.h>

AntWrapper::AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api, fly::Camera* camera, fly::CameraController* camera_controller, fly::Entity* skydome, fly::GameTimer* game_timer, QWidget* widget)
{
  TwAddVarCB(bar, "Shadows", TwType::TW_TYPE_BOOLCPP, cbSetShadows, cbGetShadows, gs, nullptr);
  TwAddVarCB(bar, "Shadows PCF", TwType::TW_TYPE_BOOLCPP, cbSetPCF, cbGetPCF, gs, nullptr);
  TwAddVarCB(bar, "Shadows single pass", TwType::TW_TYPE_BOOLCPP, cbSetShadowsSinglePass, cbGetShadowsSinglePass, gs, nullptr);
  TwAddVarCB(bar, "Shadow darken factor", TwType::TW_TYPE_FLOAT, setShadowFactor, getShadowFactor, gs, "step = 0.005f");
  TwAddVarCB(bar, "Shadow polygon offset factor", TwType::TW_TYPE_FLOAT, setSMPOFactor, getSMPOFactor, gs, "step=0.01f");
  TwAddVarCB(bar, "Shadow polygon offset units", TwType::TW_TYPE_FLOAT, setSMPOUnits, getSMPOUnits, gs, "step=0.01f");
  TwAddVarCB(bar, "Normal mapping", TwType::TW_TYPE_BOOLCPP, cbSetNormalMapping, cbGetNormalMapping, gs, nullptr);
  TwAddVarCB(bar, "Parallax mapping", TwType::TW_TYPE_BOOLCPP, cbSetParallaxMapping, cbGetParallaxMapping, gs, nullptr);
  TwAddVarCB(bar, "Relief mapping", TwType::TW_TYPE_BOOLCPP, cbSetReliefMapping, cbGetReliefMapping, gs, nullptr);
  TwAddVarCB(bar, "Depth pre pass", TwType::TW_TYPE_BOOLCPP, cbSetDepthPrepass, cbGetDepthPrepass, gs, nullptr);
  TwAddVarCB(bar, "Wind animations", TwType::TW_TYPE_BOOLCPP, cbSetWindAnimations, cbGetWindAnimations, gs, nullptr);
  TwAddVarCB(bar, "Shadow map resolution", TwType::TW_TYPE_UINT32, cbSetShadowmapsize, cbGetShadowmapsize, gs, nullptr);
  TwAddVarCB(bar, "Exposure", TwType::TW_TYPE_FLOAT, cbSetExposure, cbGetExposure, gs, "step = 0.01f");
  TwAddVarCB(bar, "Gamma", TwType::TW_TYPE_FLOAT, cbSetGamma, cbGetGamma, gs, "step = 0.01f");
  TwAddVarCB(bar, "Gamma enabled", TwType::TW_TYPE_BOOLCPP, cbSetGammaEnabled, cbGetGammaEnabled, gs, nullptr);
  TwAddVarCB(bar, "Debug quadtree", TwType::TW_TYPE_BOOLCPP, cbSetDebugQuadtree, cbGetDebugQuadtree, gs, nullptr);
  TwAddVarCB(bar, "Debug object AABBs", TwType::TW_TYPE_BOOLCPP, cbSetDebugAABBs, cbGetDebugAABBs, gs, nullptr);
  TwAddVarCB(bar, "Camera lerping", TwType::TW_TYPE_BOOLCPP, setCameraLerping, getCameraLerping, gs, nullptr);
  TwAddVarCB(bar, "Camera lerp amount", TwType::TW_TYPE_FLOAT, setCameraLerpAmount, getCameraLerpAmount, gs, "step=0.001f");
  TwAddVarCB(bar, "Detail culling threshold", TwType::TW_TYPE_FLOAT, setDetailCullingThreshold, getDetailCullingThreshold, camera, "step=0.000005f");
  TwAddVarCB(bar, "Camera speed", TwType::TW_TYPE_FLOAT, setCamSpeed, getCamSpeed, camera_controller, "step=0.1f");
  TwAddVarCB(bar, "Skydome", TwType::TW_TYPE_BOOLCPP, setSkydome, getSkydome, skydome, nullptr);
  TwAddButton(bar, "Reload shaders", cbReloadShaders, api, nullptr);
  TwAddVarCB(bar, "Depth of Field", TwType::TW_TYPE_BOOLCPP, setDepthOfField, getDepthOfField, gs, nullptr);
  TwAddVarCB(bar, "DOF near", TwType::TW_TYPE_FLOAT, setDofNear, getDofNear, gs, "step = 0.01f");
  TwAddVarCB(bar, "DOF center", TwType::TW_TYPE_FLOAT, setDofCenter, getDofCenter, gs, "step = 0.01f");
  TwAddVarCB(bar, "DOF far", TwType::TW_TYPE_FLOAT, setDofFar, getDofFar, gs, "step = 0.01f");
  TwAddVarCB(bar, "Blur radius", TwType::TW_TYPE_UINT32, setBlurRadius, getBlurRadius, gs, nullptr);
  TwAddVarCB(bar, "Blur sigma", TwType::TW_TYPE_FLOAT, setBlurSigma, getBlurSigma, gs, "step = 0.01f");
  TwAddVarCB(bar, "DOF scale factor", TwType::TW_TYPE_FLOAT, setDofScaleFactor, getDofScaleFactor, gs, "step = 0.01f");
  TwAddVarCB(bar, "Game paused", TwType::TW_TYPE_BOOLCPP, setGamePaused, getGamePaused, game_timer, nullptr);
  TwAddVarCB(bar, "Screen space reflections", TwType::TW_TYPE_BOOLCPP, setSSR, getSSR, gs, nullptr);
  TwAddVarCB(bar, "SSR steps", TwType::TW_TYPE_FLOAT, setSSRSteps, getSSRSteps, gs, "step = 0.1f");
  TwAddVarCB(bar, "SSR binary steps", TwType::TW_TYPE_UINT32, setSSRBinarySteps, getSSRBinarySteps, gs, nullptr);
  TwAddVarCB(bar, "SSR ray len scale", TwType::TW_TYPE_FLOAT, setSSRRayLenScale, getSSRRayLenScale, gs, "step=0.1f");
  TwAddVarCB(bar, "SSR min ray len", TwType::TW_TYPE_FLOAT, setSSRMinRayLen, getSSRMinRayLen, gs, "step=0.1f");
  TwAddVarCB(bar, "SSR blend weight", TwType::TW_TYPE_FLOAT, setSSRBlendWeight, getSSRBlendWeight, gs, "step=0.005f");
  TwAddVarCB(bar, "Fullscreen", TwType::TW_TYPE_BOOLCPP, setFullScreen, getFullScreen, widget, nullptr);
}

void AntWrapper::cbSetShadows(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadows(*cast<bool>(value));
}

void AntWrapper::cbGetShadows(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getShadows();
}

void AntWrapper::cbSetPCF(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowsPCF(*cast<bool>(value));
}

void AntWrapper::cbGetPCF(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowsPCF();
}

void AntWrapper::cbSetShadowsSinglePass(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSinglePassShadows(*cast<bool>(value));
}

void AntWrapper::cbGetShadowsSinglePass(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getSinglePassShadows();
}

void AntWrapper::cbSetNormalMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setNormalMapping(*cast<bool>(value));
}

void AntWrapper::cbGetNormalMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getNormalMapping();
}

void AntWrapper::cbSetParallaxMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setParallaxMapping(*cast<bool>(value));
}

void AntWrapper::cbGetParallaxMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getParallaxMapping();
}

void AntWrapper::cbSetReliefMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setReliefMapping(*cast<bool>(value));
}

void AntWrapper::cbGetReliefMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getReliefMapping();
}

void AntWrapper::cbSetDepthPrepass(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDepthprepassEnabled(*cast<bool>(value));
}

void AntWrapper::cbGetDepthPrepass(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->depthPrepassEnabled();
}

void AntWrapper::cbSetWindAnimations(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setWindAnimations(*cast<bool>(value));
}

void AntWrapper::cbGetWindAnimations(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getWindAnimations();
}

void AntWrapper::cbSetShadowmapsize(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowMapSize(*cast<unsigned>(value));
}

void AntWrapper::cbGetShadowmapsize(void * value, void * client_data)
{
  *cast<unsigned>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowMapSize();
}

void AntWrapper::cbSetExposure(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setExposure(*cast<float>(value));
}

void AntWrapper::cbGetExposure(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getExposure();
}

void AntWrapper::cbSetGammaEnabled(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGammaCorrectionEnabled(*cast<bool>(value));
}

void AntWrapper::cbGetGammaEnabled(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->gammaEnabled();
}

void AntWrapper::cbSetGamma(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGamma(*cast<float>(value));
}

void AntWrapper::cbGetGamma(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getGamma();
}

void AntWrapper::cbSetDebugQuadtree(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDebugQuadtreeNodeAABBs(*cast<bool>(value));
}

void AntWrapper::cbGetDebugQuadtree(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDebugQuadtreeNodeAABBs();
}

void AntWrapper::cbSetDebugAABBs(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDebugObjectAABBs(*cast<bool>(value));
}

void AntWrapper::cbGetDebugAABBs(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDebugObjectAABBs();
}

void AntWrapper::cbReloadShaders(void * client_data)
{
  cast<fly::OpenGLAPI>(client_data)->reloadShaders();
}

void AntWrapper::setCameraLerping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setCameraLerping(*cast<bool>(value));
}

void AntWrapper::getCameraLerping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getCameraLerping();
}

void AntWrapper::setCameraLerpAmount(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setCameraLerping(*cast<float>(value));
}

void AntWrapper::getCameraLerpAmount(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getCameraLerpAlpha();
}

void AntWrapper::setDetailCullingThreshold(const void * value, void * client_data)
{
  cast<fly::Camera>(client_data)->setDetailCullingThreshold(*cast<float>(value));
}

void AntWrapper::getDetailCullingThreshold(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::Camera>(client_data)->getDetailCullingThreshold();
}

void AntWrapper::setCamSpeed(const void * value, void * client_data)
{
  cast<fly::CameraController>(client_data)->setSpeed(*cast<float>(value));
}

void AntWrapper::getCamSpeed(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::CameraController>(client_data)->getSpeed();
}

void AntWrapper::setSkydome(const void * value, void * client_data)
{
  fly::Entity* e = cast<fly::Entity>(client_data);
  if (*cast<bool>(value)) {
    e->addComponent(std::make_shared<fly::SkydomeRenderable>(e->getComponent<fly::Model>()->getMeshes().front()));
  }
  else {
    e->removeComponent<fly::SkydomeRenderable>();
  }
}

void AntWrapper::getSkydome(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::Entity>(client_data)->getComponent<fly::SkydomeRenderable>() != nullptr;
}

void AntWrapper::setShadowFactor(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowDarkenFactor(*cast<float>(value));
}

void AntWrapper::getShadowFactor(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowDarkenFactor();
}

void AntWrapper::setDepthOfField(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDepthOfField(*cast<bool>(value));
}

void AntWrapper::getDepthOfField(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDepthOfField();
}

void AntWrapper::setDofNear(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDofNear(*cast<float>(value));
}

void AntWrapper::getDofNear(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getDofNear();
}

void AntWrapper::setDofCenter(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDofCenter(*cast<float>(value));
}

void AntWrapper::getDofCenter(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getDofCenter();
}

void AntWrapper::setDofFar(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDofFar(*cast<float>(value));
}

void AntWrapper::getDofFar(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getDofFar();
}

void AntWrapper::setBlurRadius(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setBlurRadius(*cast<unsigned>(value));
}

void AntWrapper::getBlurRadius(void * value, void * client_data)
{
  *cast<unsigned>(value) = cast<fly::GraphicsSettings>(client_data)->getBlurRadius();
}

void AntWrapper::setBlurSigma(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setBlurSigma(*cast<float>(value));
}

void AntWrapper::getBlurSigma(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getBlurSigma();
}

void AntWrapper::setDofScaleFactor(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDepthOfFieldScaleFactor(*cast<float>(value));
}

void AntWrapper::getDofScaleFactor(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getDepthOfFieldScaleFactor();
}

void AntWrapper::setGamePaused(const void * value, void * client_data)
{
  auto gt = cast<fly::GameTimer>(client_data);
  *cast<bool>(value) ? gt->stop() : gt->start();
}

void AntWrapper::getGamePaused(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GameTimer>(client_data)->isStopped();
}

void AntWrapper::setSSR(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setScreenSpaceReflections(*cast<bool>(value));
}

void AntWrapper::getSSR(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getScreenSpaceReflections();
}

void AntWrapper::setSSRSteps(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSSRSteps(*cast<float>(value));
}

void AntWrapper::getSSRSteps(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getSSRSteps();
}

void AntWrapper::setSSRBinarySteps(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSSRBinarySteps(*cast<unsigned>(value));
}

void AntWrapper::getSSRBinarySteps(void * value, void * client_data)
{
  *cast<unsigned>(value) = cast<fly::GraphicsSettings>(client_data)->getSSRBinarySteps();
}

void AntWrapper::setSSRRayLenScale(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSSRRayLenScale(*cast<float>(value));
}

void AntWrapper::getSSRRayLenScale(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getSSRRayLenScale();
}

void AntWrapper::setSSRMinRayLen(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSSRMinRayLen(*cast<float>(value));
}

void AntWrapper::getSSRMinRayLen(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getSSRMinRayLen();
}

void AntWrapper::setSSRBlendWeight(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setSSRBlendWeight(*cast<float>(value));
}

void AntWrapper::getSSRBlendWeight(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getSSRBlendWeight();
}

void AntWrapper::setFullScreen(const void * value, void * client_data)
{
  *cast<bool>(value) ? cast<QWidget>(client_data)->showFullScreen() : cast<QWidget>(client_data)->showNormal();
}

void AntWrapper::getFullScreen(void * value, void * client_data)
{
  *cast<bool>(value) = cast<QWidget>(client_data)->isFullScreen();
}

void AntWrapper::setSMPOFactor(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowPolygonOffsetFactor(*cast<float>(value));
}

void AntWrapper::setSMPOUnits(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowPolygonOffsetUnits(*cast<float>(value));
}

void AntWrapper::getSMPOFactor(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowPolygonOffsetFactor();
}

void AntWrapper::getSMPOUnits(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowPolygonOffsetUnits();
}
