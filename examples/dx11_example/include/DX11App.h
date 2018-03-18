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

#define SPONZA 1
#define SPONZA_MULTIPLE 0

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
  float _camSpeed = 3.f;
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

  void drawDebugGUI();

  static void TW_CALL TwSetPtrFrequCallback(const void* value, void* client_data);
  static void TW_CALL TwGetPtrFrequCallback(void* value, void* client_data);
  static void TW_CALL TwSetPtrHeightScaleCallback(const void* value, void* client_data);
  static void TW_CALL TwGetPtrHeightScaleCallback(void* value, void* client_data);
  static void TW_CALL TwSetPtrOctavesCallback(const void* value, void* client_data);
  static void TW_CALL TwGetPtrOctavesCallback(void* value, void* client_data);
  static void TW_CALL TwSetWireframeCallback(const void* value, void* client_data);
  static void TW_CALL TwGetWireframeCallback(void* value, void* client_data);
  static void TW_CALL TwSetAmpScaleCb(const void* value, void* client_data);
  static void TW_CALL TwGetAmpScaleCb(void* value, void* client_data);
  static void TW_CALL TwSetFrequencyScaleCb(const void* value, void* client_data);
  static void TW_CALL TwGetFrequencyScaleCb(void* value, void* client_data);
  static void TW_CALL TwSetUVScaleDetailsCb(const void* value, void* client_data);
  static void TW_CALL TwGetUVScaleDetailsCb(void* value, void* client_data);
  static void TW_CALL TwSetDofNear(const void* value, void* client_data);
  static void TW_CALL TwGetDofNear(void* value, void* client_data);
  static void TW_CALL TwSetDofCenter(const void* value, void* client_data);
  static void TW_CALL TwGetDofCenter(void* value, void* client_data);
  static void TW_CALL TwSetDofFar(const void* value, void* client_data);
  static void TW_CALL TwGetDofFar(void* value, void* client_data);
  static void TW_CALL TwSetTerrainSize(const void* value, void* client_data);
  static void TW_CALL TwGetTerrainSize(void* value, void* client_data);
  static void TW_CALL TwSetMaxTessFactor(const void* value, void* client_data);
  static void TW_CALL TwGetMaxTessFactor(void* value, void* client_data);
  static void TW_CALL TwSetMaxTessDistance(const void* value, void* client_data);
  static void TW_CALL TwGetMaxTessDistance(void* value, void* client_data);
  static void TW_CALL TwSetSkycolor(const void* value, void* client_data);
  static void TW_CALL TwGetSkycolor(void* value, void* client_data);
  static void TW_CALL TwSetBrightScale(const void* value, void* client_data);
  static void TW_CALL TwGetBrightScale(void* value, void* client_data);
  static void TW_CALL TwSetBrightBias(const void* value, void* client_data);
  static void TW_CALL TwGetBrightBias(void* value, void* client_data);
  static void TW_CALL TwSetDofEnabled(const void* value, void* client_data);
  static void TW_CALL TwGetDofEnabled(void* value, void* client_data);
  static void TW_CALL TwSetLensflareEnabled(const void* value, void* client_data);
  static void TW_CALL TwGetLensflareEnabled(void* value, void* client_data);
  static void TW_CALL TwSetExposure(const void* value, void* client_data);
  static void TW_CALL TwGetExposure(void* value, void* client_data);
  static void TW_CALL TwSetSSR(const void* value, void* client_data);
  static void TW_CALL TwGetSSR(void* value, void* client_data);
  static void TW_CALL TwSetSSRWeight(const void* value, void* client_data);
  static void TW_CALL TwGetSSRWeight(void* value, void* client_data);
};

#endif // !DX11APP_H
