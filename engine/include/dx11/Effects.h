#ifndef EFFECTS_H
#define EFFECTS_H

#include <Windows.h>
#include <string>
#include <d3dx11effect.h>
#include <atlbase.h>
#include <vector>

namespace fly
{
  class Effects
  {
  public:
    enum MeshRenderFlags : unsigned
    {
      WindX = 1u, // Vertex shader uniforms
      WindZ = 2u,
      Diffuse = 4u, // Pixel shader uniforms
      Alpha = 8u,
      Normal = 16u
    };
    enum ShadowMapRenderFlags : unsigned
    {
      ShadowWindX = 1u,
      ShadowWindZ = 2u,
      ShadowAlpha = 4u
    };
    enum CompositeRenderFlags : unsigned
    {
      DepthOfField = 1u,
      Lensflare = 2u,
      MotionBlur = 4u,
      LightVolumes = 8u
    };
    Effects(const std::wstring& file, const CComPtr<ID3D11Device>& device, const std::wstring& terrain_file);
    D3DX11_PASS_DESC getMeshPassDesc();
    D3DX11_PASS_DESC getTerrainPassDesc();
    ID3DX11EffectTechnique* getMeshTechnique(MeshRenderFlags flags);
    ID3DX11EffectTechnique* getShadowMapTechnique(ShadowMapRenderFlags flags);
    ID3DX11EffectTechnique* getCompositeTechnique(CompositeRenderFlags flags);
    CComPtr<ID3DX11Effect> getEffect();
    CComPtr<ID3DX11Effect> getTerrainEffect();
  private:
    CComPtr<ID3DX11Effect> _effect;
    CComPtr<ID3DX11Effect> _terrainEffect;
    std::vector<ID3DX11EffectTechnique*> _meshTechniques;
    std::vector<ID3DX11EffectTechnique*> _shadowTechniques;
    std::vector<ID3DX11EffectTechnique*> _compositeTechniques;
    CComPtr<ID3DX11Effect> createEffectFromFile(const std::wstring& file, const CComPtr<ID3D11Device>& device);
  };
}

#endif
