#ifndef DX11STATES_H
#define DX11STATES_H

#include <d3d11.h>
#include <atlbase.h>

namespace fly
{
  class DX11States
  {
  public:
    DX11States(const CComPtr<ID3D11Device>& device);
    virtual ~DX11States() = default;

    ID3D11BlendState* blendAdditiveColor();
    ID3D11BlendState* blendNonPremultipliedBlendFactorColor();
    ID3D11DepthStencilState* depthReadWriteLessEqual();
    ID3D11DepthStencilState* depthReadWriteStencilWrite();
    ID3D11DepthStencilState* depthNoneStencilReadEqual();
    ID3D11RasterizerState* rastWireFrame();
    ID3D11RasterizerState* rastCullBackFrontCounterClockwiseFill();
    ID3D11RasterizerState* rastCullFrontFrontCounterClockwiseFill();
  private:
    CComPtr<ID3D11BlendState> _blendAdditiveColor;
    CComPtr<ID3D11BlendState> _blendNonPremultipliedBlendFactorColor;
    CComPtr<ID3D11DepthStencilState> _depthReadWriteLessEqual;
    CComPtr<ID3D11DepthStencilState> _depthReadWriteStencilWrite;
    CComPtr<ID3D11DepthStencilState> _depthNoneStencilReadEqual;
    CComPtr<ID3D11RasterizerState> _rastWireFrame;
    CComPtr<ID3D11RasterizerState> _rastCullBackFrontCounterClockwiseFill;
    CComPtr<ID3D11RasterizerState> _rastCullFrontFrontCounterClockwiseFill;
  };
}

#endif // !DX11STATES_H
