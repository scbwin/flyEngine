#include <dx11/DX11States.h>
#include <dx11/DXUtils.h>

namespace fly
{
  DX11States::DX11States(const CComPtr<ID3D11Device>& device)
  {
    D3D11_BLEND_DESC blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].BlendOp = blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].DestBlend =
      blend_desc.RenderTarget[0].DestBlendAlpha =
      blend_desc.RenderTarget[0].SrcBlend =
      blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
    HR(device->CreateBlendState(&blend_desc, &_blendAdditiveColor));

    blend_desc = {};
    blend_desc.RenderTarget[0].BlendEnable = true;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
    HR(device->CreateBlendState(&blend_desc, &_blendNonPremultipliedBlendFactorColor));

    D3D11_DEPTH_STENCIL_DESC ds_desc = {};
    ds_desc.DepthEnable = true;
    ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
    ds_desc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;
    ds_desc.StencilEnable = false;
    HR(device->CreateDepthStencilState(&ds_desc, &_depthReadWriteLessEqual));

    ds_desc.StencilEnable = true;
    ds_desc.StencilReadMask = 0xff;
    ds_desc.StencilWriteMask = 0xff;
    ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS;
    ds_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE;
    ds_desc.BackFace = ds_desc.FrontFace;
    HR(device->CreateDepthStencilState(&ds_desc, &_depthReadWriteStencilWrite));

    ds_desc.DepthEnable = false;
    ds_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
    ds_desc.FrontFace.StencilFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL;
    ds_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP;
    ds_desc.BackFace = ds_desc.FrontFace;
    HR(device->CreateDepthStencilState(&ds_desc, &_depthNoneStencilReadEqual));

    D3D11_RASTERIZER_DESC rast_desc = {};
    rast_desc.AntialiasedLineEnable = false;
    rast_desc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
    rast_desc.DepthBias = 0;
    rast_desc.DepthBiasClamp = 0.f;
    rast_desc.DepthClipEnable = true;
    rast_desc.FillMode = D3D11_FILL_MODE::D3D11_FILL_WIREFRAME;
    rast_desc.FrontCounterClockwise = true;
    rast_desc.SlopeScaledDepthBias = 0.f;
    HR(device->CreateRasterizerState(&rast_desc, &_rastWireFrame));

    rast_desc = {};
    rast_desc.AntialiasedLineEnable = false;
    rast_desc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
    rast_desc.DepthBias = 0;
    rast_desc.DepthBiasClamp = 0.f;
    rast_desc.DepthClipEnable = true;
    rast_desc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    rast_desc.FrontCounterClockwise = true;
    rast_desc.SlopeScaledDepthBias = 0.f;
    HR(device->CreateRasterizerState(&rast_desc, &_rastCullBackFrontCounterClockwiseFill));

    rast_desc.CullMode = D3D11_CULL_MODE::D3D11_CULL_FRONT;
    HR(device->CreateRasterizerState(&rast_desc, &_rastCullFrontFrontCounterClockwiseFill));
  }
  ID3D11BlendState* DX11States::blendAdditiveColor()
  {
    return _blendAdditiveColor;
  }
  ID3D11BlendState* DX11States::blendNonPremultipliedBlendFactorColor()
  {
    return _blendNonPremultipliedBlendFactorColor;
  }
  ID3D11DepthStencilState* DX11States::depthReadWriteLessEqual()
  {
    return _depthReadWriteLessEqual;
  }
  ID3D11DepthStencilState* DX11States::depthReadWriteStencilWrite()
  {
    return _depthReadWriteStencilWrite;
  }
  ID3D11DepthStencilState * DX11States::depthNoneStencilReadEqual()
  {
    return _depthNoneStencilReadEqual;
  }
  ID3D11RasterizerState * DX11States::rastWireFrame()
  {
    return _rastWireFrame;
  }
  ID3D11RasterizerState * DX11States::rastCullBackFrontCounterClockwiseFill()
  {
    return _rastCullBackFrontCounterClockwiseFill;
  }
  ID3D11RasterizerState * DX11States::rastCullFrontFrontCounterClockwiseFill()
  {
    return _rastCullFrontFrontCounterClockwiseFill;
  }
}