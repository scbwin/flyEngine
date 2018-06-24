#include <AntWrapper.h>
#include <GraphicsSettings.h>
#include <opengl/OpenGLAPI.h>
#include <Camera.h>
#include <CameraController.h>
#include <Model.h>
#include <GameTimer.h>
#include <qwidget.h>
#include <Light.h>
#include <renderer/Renderer.h>

AntWrapper::AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api, 
  fly::CameraController* camera_controller, std::shared_ptr<fly::SkydomeRenderable<fly::OpenGLAPI, fly::AABB>> skydome,
  fly::GameTimer* game_timer, QWidget* widget, std::shared_ptr<fly::Camera> camera, fly::DirectionalLight* dl,
  fly::Renderer<fly::OpenGLAPI, fly::AABB>* renderer)
{
  _skydomeData._renderer = renderer;
  _skydomeData._skydome = skydome;

  TwAddVarCB(bar, "Multithreaded culling", TwType::TW_TYPE_BOOLCPP, setMTCulling, getMTCulling, gs, nullptr);
  TwAddVarCB(bar, "Multithreaded detail culling", TwType::TW_TYPE_BOOLCPP, setMTDetailCulling, getMTDetailCulling, gs, nullptr);
  TwAddVarCB(bar, "Shadows", TwType::TW_TYPE_BOOLCPP, setShadows, getShadows, gs, nullptr);
  TwAddVarCB(bar, "Shadows PCF", TwType::TW_TYPE_BOOLCPP, setPCF, getPCF, gs, nullptr);
  TwAddVarCB(bar, "Max shadow cast distance", TwType::TW_TYPE_FLOAT, setMaxShadowCastDistance, getMaxShadowCastDistance, dl, "step = 0.5f");
  TwAddVarCB(bar, "Light dir X", TwType::TW_TYPE_FLOAT, setLightDirX, getLightDirX, dl, "step = 0.005f");
  TwAddVarCB(bar, "Light dir Y", TwType::TW_TYPE_FLOAT, setLightDirY, getLightDirY, dl, "step = 0.005f");
  TwAddVarCB(bar, "Light dir Z", TwType::TW_TYPE_FLOAT, setLightDirZ, getLightDirZ, dl, "step = 0.005f");
  TwAddVarCB(bar, "God rays", TwType::TW_TYPE_BOOLCPP, setGodRays, getGodRays, gs, nullptr);
  TwAddVarCB(bar, "God ray decay", TwType::TW_TYPE_FLOAT, setGodRayDecay, getGodRayDecay, gs, "step = 0.001f");
  TwAddVarCB(bar, "God ray steps", TwType::TW_TYPE_FLOAT, setGodRaySteps, getGodRaySteps, gs, "step = 1.f");
  TwAddVarCB(bar, "God ray fade dist", TwType::TW_TYPE_FLOAT, setGodRayFadeDist, getGodRayFadeDist, gs, "step = 0.001");
  TwAddVarCB(bar, "Shadow darken factor", TwType::TW_TYPE_FLOAT, setShadowFactor, getShadowFactor, gs, "step = 0.005f");
  TwAddVarCB(bar, "Shadow polygon offset factor", TwType::TW_TYPE_FLOAT, setSMPOFactor, getSMPOFactor, gs, "step=0.01f");
  TwAddVarCB(bar, "Shadow polygon offset units", TwType::TW_TYPE_FLOAT, setSMPOUnits, getSMPOUnits, gs, "step=0.01f");
  TwAddVarCB(bar, "Normal mapping", TwType::TW_TYPE_BOOLCPP, setNormalMapping, getNormalMapping, gs, nullptr);
  TwAddVarCB(bar, "Parallax mapping", TwType::TW_TYPE_BOOLCPP, setParallaxMapping, getParallaxMapping, gs, nullptr);
  TwAddVarCB(bar, "Relief mapping", TwType::TW_TYPE_BOOLCPP, setReliefMapping, getReliefMapping, gs, nullptr);
  TwAddVarCB(bar, "Depth pre pass", TwType::TW_TYPE_BOOLCPP, setDepthPrepass, getDepthPrepass, gs, nullptr);
  TwAddVarCB(bar, "Shadow map resolution", TwType::TW_TYPE_UINT32, setShadowmapsize, getShadowmapsize, gs, nullptr);
  TwAddVarCB(bar, "Exposure", TwType::TW_TYPE_FLOAT, setExposure, getExposure, gs, "step = 0.01f");
  TwAddVarCB(bar, "Gamma", TwType::TW_TYPE_FLOAT, setGamma, getGamma, gs, "step = 0.01f");
  TwAddVarCB(bar, "Gamma enabled", TwType::TW_TYPE_BOOLCPP, setGammaEnabled, getGammaEnabled, gs, nullptr);
  TwAddVarCB(bar, "Debug BVH", TwType::TW_TYPE_BOOLCPP, setDebugBVH, getDebugBVH, gs, nullptr);
  TwAddVarCB(bar, "Debug object AABBs", TwType::TW_TYPE_BOOLCPP, setDebugAABBs, getDebugAABBs, gs, nullptr);
  TwAddVarCB(bar, "Detail culling threshold", TwType::TW_TYPE_FLOAT, setDetailCullingThreshold, getDetailCullingThreshold, camera_controller, "step=0.0000005f");
  TwAddVarCB(bar, "Lod range multiplier", TwType::TW_TYPE_FLOAT, setLodRangeMultiplier, getLodRangeMultiplier, camera_controller, "step=0.1f");
  TwAddVarCB(bar, "Camera speed", TwType::TW_TYPE_FLOAT, setCamSpeed, getCamSpeed, camera_controller, "step=0.1f");
  TwAddVarCB(bar, "Skydome", TwType::TW_TYPE_BOOLCPP, setSkydome, getSkydome, &_skydomeData, nullptr);
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

void AntWrapper::setShadows(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadows(*cast<bool>(value));
}

void AntWrapper::getShadows(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getShadows();
}

void AntWrapper::setPCF(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowsPCF(*cast<bool>(value));
}

void AntWrapper::getPCF(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowsPCF();
}
void AntWrapper::setLightDirX(const void * value, void * client_data)
{
  auto direction = cast<fly::DirectionalLight>(client_data)->getDirection();
  direction[0] = *cast<float>(value);
  cast<fly::DirectionalLight>(client_data)->setDirection(direction);
}
void AntWrapper::getLightDirX(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::DirectionalLight>(client_data)->getDirection()[0];
}
void AntWrapper::setLightDirY(const void * value, void * client_data)
{
  auto direction = cast<fly::DirectionalLight>(client_data)->getDirection();
  direction[1] = *cast<float>(value);
  cast<fly::DirectionalLight>(client_data)->setDirection(direction);
}
void AntWrapper::getLightDirY(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::DirectionalLight>(client_data)->getDirection()[1];
}
void AntWrapper::setLightDirZ(const void * value, void * client_data)
{
  auto direction = cast<fly::DirectionalLight>(client_data)->getDirection();
  direction[2] = *cast<float>(value);
  cast<fly::DirectionalLight>(client_data)->setDirection(direction);
}
void AntWrapper::getLightDirZ(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::DirectionalLight>(client_data)->getDirection()[2];
}
void AntWrapper::setGodRays(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGodRays(*cast<bool>(value));
}
void AntWrapper::getGodRays(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getGodRays();
}
void AntWrapper::setGodRayDecay(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGodRayDecay(*cast<float>(value));
}
void AntWrapper::getGodRayDecay(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getGodRayDecay();
}
void AntWrapper::setGodRaySteps(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGodRaySteps(*cast<float>(value));
}
void AntWrapper::getGodRaySteps(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getGodRaySteps();
}
void AntWrapper::setGodRayFadeDist(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGodRayFadeDist(*cast<float>(value));
}
void AntWrapper::getGodRayFadeDist(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getGodRayFadeDist();
}
void AntWrapper::setMaxShadowCastDistance(const void * value, void * client_data)
{
  cast<fly::DirectionalLight>(client_data)->setMaxShadowCastDistance(*cast<float>(value));
}
void AntWrapper::getMaxShadowCastDistance(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::DirectionalLight>(client_data)->getMaxShadowCastDistance();
}
void AntWrapper::setNormalMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setNormalMapping(*cast<bool>(value));
}

void AntWrapper::getNormalMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getNormalMapping();
}

void AntWrapper::setParallaxMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setParallaxMapping(*cast<bool>(value));
}

void AntWrapper::getParallaxMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getParallaxMapping();
}

void AntWrapper::setReliefMapping(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setReliefMapping(*cast<bool>(value));
}

void AntWrapper::getReliefMapping(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getReliefMapping();
}

void AntWrapper::setDepthPrepass(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDepthprepassEnabled(*cast<bool>(value));
}

void AntWrapper::getDepthPrepass(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->depthPrepassEnabled();
}

void AntWrapper::setShadowmapsize(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setShadowMapSize(*cast<unsigned>(value));
}

void AntWrapper::getShadowmapsize(void * value, void * client_data)
{
  *cast<unsigned>(value) = cast<fly::GraphicsSettings>(client_data)->getShadowMapSize();
}

void AntWrapper::setExposure(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setExposure(*cast<float>(value));
}

void AntWrapper::getExposure(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getExposure();
}

void AntWrapper::setGammaEnabled(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGammaCorrectionEnabled(*cast<bool>(value));
}

void AntWrapper::getGammaEnabled(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->gammaEnabled();
}

void AntWrapper::setGamma(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setGamma(*cast<float>(value));
}

void AntWrapper::getGamma(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::GraphicsSettings>(client_data)->getGamma();
}

void AntWrapper::setDebugBVH(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDebugBVH(*cast<bool>(value));
}

void AntWrapper::getDebugBVH(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDebugBVH();
}

void AntWrapper::setDebugAABBs(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDebugObjectBVs(*cast<bool>(value));
}

void AntWrapper::getDebugAABBs(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDebugObjectBVs();
}
void AntWrapper::setDetailCullingThreshold(const void * value, void * client_data)
{
  cast<fly::CameraController>(client_data)->getCamera()->setDetailCullingThreshold(*cast<float>(value));
}

void AntWrapper::getDetailCullingThreshold(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::CameraController>(client_data)->getCamera()->getDetailCullingThreshold();
}

void AntWrapper::setLodRangeMultiplier(const void * value, void * client_data)
{
  cast<fly::CameraController>(client_data)->getCamera()->setLodRangeMultiplier(*cast<float>(value));
}

void AntWrapper::getLodRangeMultiplier(void * value, void * client_data)
{
  *cast<float>(value) = cast<fly::CameraController>(client_data)->getCamera()->getLodRangeMultiplier();
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
  cast<SkydomeData>(client_data)->_renderer->setSkydome(*cast<bool>(value) ? cast<SkydomeData>(client_data)->_skydome : nullptr);
}

void AntWrapper::getSkydome(void * value, void * client_data)
{
  *cast<bool>(value) = cast<SkydomeData>(client_data)->_renderer->getSkydome() != nullptr;
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

void AntWrapper::setMTCulling(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setMultithreadedCulling(*cast<bool>(value));
}

void AntWrapper::getMTCulling(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getMultithreadedCulling();
}
void AntWrapper::setMTDetailCulling(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setMultithreadedDetailCulling(*cast<bool>(value));
}

void AntWrapper::getMTDetailCulling(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getMultithreadedDetailCulling();
}