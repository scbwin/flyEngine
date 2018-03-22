#ifndef DX11APP_H
#define DX11APP_H

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <windows.h>
#include <Engine.h>
#include <EntityManager.h>
#include <dx11/RenderingSystemDX11.h>
#include <memory>
#include <Camera.h>
#include <GameTimer.h>
#include <AnimationSystem.h>
#include <dx11/debug.h>
#include <CommonStates.h>
#include <AntTweakBar.h>
#include <SpriteFont.h>

#define SPONZA 1
#define SPONZA_MULTIPLE 1

class DX11App
{
public:
  DX11App();
  virtual ~DX11App();
  int execute();
private:
  void initWindow();
  void initGame();
  void handleInput();
  bool keyPressed(int key);
  float getCamSpeed();
  static LRESULT CALLBACK windowCallbackStatic(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
  LRESULT windowCallback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
  HWND _window;
  glm::uvec2 _windowSize;
  std::unique_ptr<fly::Engine> _engine;
  std::shared_ptr<fly::RenderingSystemDX11> _rs;
  std::shared_ptr<fly::Camera> _camera;
  std::shared_ptr<fly::DirectionalLight> _dl;
 // std::shared_ptr<fly::Transform> _dlTransform;
  fly::GameTimer _gameTimer;
#if SPONZA
  float _camSpeed = 300.f;
#else
  float _camSpeed = 300.f;
#endif
  float _camSpeedAccFactor = 2.f;
  float _camSpeedDecFactor = 0.6f;
  void onKeyUp(WPARAM w_param, LPARAM l_param);
  void onMouseMove(WPARAM w_param, LPARAM l_param);
  glm::vec2 _mousePosBefore = glm::vec2(0.f);

  std::unique_ptr<DirectX::SpriteFont> _font;
  std::unique_ptr<DirectX::SpriteBatch> _spriteBatch;
  std::wstring _adapterString;
  std::wstring _debugString;
  std::wstring _dbgString;
  float _dbgStringAlpha = 0.f;
 // CComPtr<ID3D11BlendState> _textBlendState;
  std::unique_ptr<DirectX::CommonStates> _commonStates;
  bool _showGUI = true;
#if DX11_STATS
  std::wstring getStatsString();
  std::wstring formatNumber(unsigned number);
#endif
  void drawDebugGUI();

  static void TwSetPtrFrequCallback(const void* value, void* client_data);
  static void TwGetPtrFrequCallback(void* value, void* client_data);
  static void TwSetPtrHeightScaleCallback(const void* value, void* client_data);
  static void TwGetPtrHeightScaleCallback(void* value, void* client_data);
  static void TwSetPtrOctavesCallback(const void* value, void* client_data);
  static void TwGetPtrOctavesCallback(void* value, void* client_data);
  static void TwSetWireframeCallback(const void* value, void* client_data);
  static void TwGetWireframeCallback(void* value, void* client_data);
  static void TwSetAmpScaleCb(const void* value, void* client_data);
  static void TwGetAmpScaleCb(void* value, void* client_data);
  static void TwSetFrequencyScaleCb(const void* value, void* client_data);
  static void TwGetFrequencyScaleCb(void* value, void* client_data);
  static void TwSetUVScaleDetailsCb(const void* value, void* client_data);
  static void TwGetUVScaleDetailsCb(void* value, void* client_data);
  static void TwSetDofNear(const void* value, void* client_data);
  static void TwGetDofNear(void* value, void* client_data);
  static void TwSetDofCenter(const void* value, void* client_data);
  static void TwGetDofCenter(void* value, void* client_data);
  static void TwSetDofFar(const void* value, void* client_data);
  static void TwGetDofFar(void* value, void* client_data);
  static void TwSetTerrainSize(const void* value, void* client_data);
  static void TwGetTerrainSize(void* value, void* client_data);
  static void TwSetMaxTessFactor(const void* value, void* client_data);
  static void TwGetMaxTessFactor(void* value, void* client_data);
  static void TwSetMaxTessDistance(const void* value, void* client_data);
  static void TwGetMaxTessDistance(void* value, void* client_data);
  static void TwSetSkycolor(const void* value, void* client_data);
  static void TwGetSkycolor(void* value, void* client_data);
  static void TwSetBrightScale(const void* value, void* client_data);
  static void TwGetBrightScale(void* value, void* client_data);
  static void TwSetBrightBias(const void* value, void* client_data);
  static void TwGetBrightBias(void* value, void* client_data);
  static void TwSetDofEnabled(const void* value, void* client_data);
  static void TwGetDofEnabled(void* value, void* client_data);
  static void TwSetLensflareEnabled(const void* value, void* client_data);
  static void TwGetLensflareEnabled(void* value, void* client_data);
  static void TwSetExposure(const void* value, void* client_data);
  static void TwGetExposure(void* value, void* client_data);
  static void TwSetSSR(const void* value, void* client_data);
  static void TwGetSSR(void* value, void* client_data);
  static void TwSetSSRWeight(const void* value, void* client_data);
  static void TwGetSSRWeight(void* value, void* client_data);
  static void TwSetSSRSteps(const void* value, void* client_data);
  static void TwGetSSRSteps(void* value, void* client_data);
  static void TwSetSSRMinRayLen(const void* value, void* client_data);
  static void TwGetSSRMinRayLen(void* value, void* client_data);
  static void TwSetSSRRayLenScale(const void* value, void* client_data);
  static void TwGetSSRRayLenScale(void* value, void* client_data);
  static void TwSetSmDepthBias(const void* value, void* client_data);
  static void TwGetSmDepthBias(void* value, void* client_data);
  static void TwSetSmSlopeScaledDepthBias(const void* value, void* client_data);
  static void TwGetSmSlopeScaledDepthBias(void* value, void* client_data);
  static void TwSetDetailCullingErrorThreshold(const void* value, void* client_data);
  static void TwGetDetailCullingErrorThreshold(void* value, void* client_data);
  static void TwSetDetailCullingErrorExponent(const void* value, void* client_data);
  static void TwGetDetailCullingErrorExponent(void* value, void* client_data);
};

#endif // !DX11APP_H
