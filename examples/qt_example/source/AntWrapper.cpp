#include <AntWrapper.h>
#include <GraphicsSettings.h>
#include <opengl/OpenGLAPI.h>
#include <Camera.h>
#include <CameraController.h>

AntWrapper::AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api, fly::Camera* camera, fly::CameraController* camera_controller)
{
  TwAddVarCB(bar, "Shadows", TwType::TW_TYPE_BOOLCPP, cbSetShadows, cbGetShadows, gs, nullptr);
  TwAddVarCB(bar, "Shadows PCF", TwType::TW_TYPE_BOOLCPP, cbSetPCF, cbGetPCF, gs, nullptr);
  TwAddVarCB(bar, "Normal mapping", TwType::TW_TYPE_BOOLCPP, cbSetNormalMapping, cbGetNormalMapping, gs, nullptr);
  TwAddVarCB(bar, "Parallax mapping", TwType::TW_TYPE_BOOLCPP, cbSetParallaxMapping, cbGetParallaxMapping, gs, nullptr);
  TwAddVarCB(bar, "Relief mapping", TwType::TW_TYPE_BOOLCPP, cbSetReliefMapping, cbGetReliefMapping, gs, nullptr);
  TwAddVarCB(bar, "Depth pre pass", TwType::TW_TYPE_BOOLCPP, cbSetDepthPrepass, cbGetDepthPrepass, gs, nullptr);
  TwAddVarCB(bar, "Wind animations", TwType::TW_TYPE_BOOLCPP, cbSetWindAnimations, cbGetWindAnimations, gs, nullptr);
  TwAddVarCB(bar, "Shadow map resolution", TwType::TW_TYPE_UINT32, cbSetShadowmapsize, cbGetShadowmapsize, gs, nullptr);
  TwAddVarCB(bar, "Exposure", TwType::TW_TYPE_FLOAT, cbSetExposure, cbGetExposure, gs, "step = 0.01f");
  TwAddVarCB(bar, "Debug quadtree", TwType::TW_TYPE_BOOLCPP, cbSetDebugQuadtree, cbGetDebugQuadtree, gs, nullptr);
  TwAddVarCB(bar, "Debug object AABBs", TwType::TW_TYPE_BOOLCPP, cbSetDebugAABBs, cbGetDebugAABBs, gs, nullptr);
  TwAddVarCB(bar, "Camera lerping", TwType::TW_TYPE_BOOLCPP, setCameraLerping, getCameraLerping, gs, nullptr);
  TwAddVarCB(bar, "Camera lerp amount", TwType::TW_TYPE_FLOAT, setCameraLerpAmount, getCameraLerpAmount, gs, "step=0.001f");
  TwAddVarCB(bar, "Detail culling", TwType::TW_TYPE_BOOLCPP, setDetailCulling, getDetailCulling, gs, nullptr);
  TwAddVarCB(bar, "Detail culling threshold", TwType::TW_TYPE_FLOAT, setDetailCullingThreshold, getDetailCullingThreshold, camera, "step=0.000005f");
  TwAddVarCB(bar, "Camera speed", TwType::TW_TYPE_FLOAT, setCamSpeed, getCamSpeed, camera_controller, "step=0.1f");
  TwAddButton(bar, "Reload shaders", cbReloadShaders, api, nullptr);
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

void AntWrapper::setDetailCulling(const void * value, void * client_data)
{
  cast<fly::GraphicsSettings>(client_data)->setDetailCulling(*cast<bool>(value));
}

void AntWrapper::getDetailCulling(void * value, void * client_data)
{
  *cast<bool>(value) = cast<fly::GraphicsSettings>(client_data)->getDetailCulling();
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
