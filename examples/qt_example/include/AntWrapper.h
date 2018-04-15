#ifndef ANTWRAPPER_H
#define ANTWRAPPER_H

#include <AntTweakBar.h>

namespace fly
{
  class GraphicsSettings;
  class OpenGLAPI;
}

class AntWrapper
{
public:
  AntWrapper(TwBar* bar, fly::GraphicsSettings* gs, fly::OpenGLAPI* api);
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
  static void cbSetDebugQuadtree(const void* value, void* client_data);
  static void cbGetDebugQuadtree(void* value, void* client_data);
  static void cbSetDebugAABBs(const void* value, void* client_data);
  static void cbGetDebugAABBs(void* value, void* client_data);
  static void cbReloadShaders(void* client_data);
  template<typename T> static T* cast(void* data){return reinterpret_cast<T*>(data);}
  template<typename T> static const T* cast(const void* data) { return reinterpret_cast<const T*>(data); }
};

#endif // !ANTWRAPPER_H
