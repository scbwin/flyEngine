#include <dx11/Effects.h>
#include <fstream>
#include <sstream>
#include <d3dcompiler.h>
#include <dx11/DXUtils.h>

namespace fly
{
  Effects::Effects(const std::wstring & file, const CComPtr<ID3D11Device>& device, const std::wstring& terrain_file)
  {
    _terrainEffect = createEffectFromFile(terrain_file, device);

    std::ifstream is(file);
    std::stringstream ss;
    ss << is.rdbuf();
    std::string file_contents = ss.str();

    std::string str = file_contents;
    auto add_line = [&str](const std::string& app) {
      str.append(app + "\n");
    };

    unsigned all_mesh_flags = 0;
    for (unsigned i = 0; i <= static_cast<unsigned>(MeshRenderFlags::Normal); i++) {
      all_mesh_flags |= i;
    }

    for (unsigned int i = 0; i <= all_mesh_flags; i++) {
      auto to_str = [i](unsigned flag) {
        return std::string(i & flag ? "true" : "false");
      };
      add_line("technique11 meshTech" + std::to_string(i));
      add_line("{");
      add_line("  pass pass0");
      add_line("  {");
      add_line("    SetVertexShader(CompileShader(vs_5_0, vertexShader(" + to_str( MeshRenderFlags::WindX) + "," + to_str(MeshRenderFlags::WindZ) + ")));");
      add_line("    SetPixelShader(CompileShader(ps_5_0, pixelShader(" + to_str(MeshRenderFlags::Diffuse) + "," + to_str(MeshRenderFlags::Alpha) + "," + to_str(MeshRenderFlags::Normal) + ")));");
      add_line("   }");
      add_line("}");
    }

    unsigned all_shadow_flags = 0;
    for (unsigned i = 0; i <= static_cast<unsigned>(ShadowMapRenderFlags::ShadowAlpha); i++) {
      all_shadow_flags |= i;
    }

    for (unsigned i = 0; i <= all_shadow_flags; i++) {
      auto to_str = [i](unsigned flag) {
        return std::string(i & flag ? "true" : "false");
      };
      add_line("technique11 shadowTech" + std::to_string(i));
      add_line("{");
      add_line("  pass pass0");
      add_line("  {");
      add_line("    SetVertexShader(CompileShader(vs_5_0, vsShadowMap(" + to_str(ShadowMapRenderFlags::ShadowWindX) + "," + to_str(ShadowMapRenderFlags::ShadowWindZ) + ")));");
      add_line("    SetPixelShader(CompileShader(ps_5_0, psShadowMap(" + to_str(ShadowMapRenderFlags::ShadowAlpha) + ")));");
      add_line("   }");
      add_line("}");
    }

    unsigned all_composite_flags = 0;
    for (unsigned i = 0; i <= CompositeRenderFlags::LightVolumes; i++) {
      all_composite_flags |= i;
    }

    for (unsigned int i = 0; i <= all_composite_flags; i++) {
      auto to_str = [i](unsigned flag) {
        return std::string(i & flag ? "true" : "false");
      };
      add_line("technique11 compositeTech" + std::to_string(i));
      add_line("{");
      add_line("  pass pass0");
      add_line("  {");
      add_line("    SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));");
      add_line("    SetPixelShader(CompileShader(ps_5_0, psComposite(" + to_str(DepthOfField) + "," + to_str(Lensflare) + "," + to_str(MotionBlur) + "," + to_str(LightVolumes) + ")));");
      add_line("   }");
      add_line("}");
    }

    std::ofstream os("temp_effects.fx");
    os.write(str.c_str(), str.size());
    os.close();

    _effect = createEffectFromFile(L"temp_effects.fx", device);

    for (unsigned i = 0; i <= all_mesh_flags; i++) {
      std::string name = "meshTech" + std::to_string(i);
      _meshTechniques.push_back(_effect->GetTechniqueByName(name.c_str()));
    }
    for (unsigned i = 0; i <= all_shadow_flags; i++) {
      std::string name = "shadowTech" + std::to_string(i);
      _shadowTechniques.push_back(_effect->GetTechniqueByName(name.c_str()));
    }
    for (unsigned i = 0; i <= all_composite_flags; i++) {
      std::string name = "compositeTech" + std::to_string(i);
      _compositeTechniques.push_back(_effect->GetTechniqueByName(name.c_str()));
    }
  }
  D3DX11_PASS_DESC Effects::getMeshPassDesc()
  {
    D3DX11_PASS_DESC ret = {};
    _meshTechniques.front()->GetPassByIndex(0)->GetDesc(&ret);
    return ret;
  }
  D3DX11_PASS_DESC Effects::getTerrainPassDesc()
  {
    D3DX11_PASS_DESC ret = {};
    _terrainEffect->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&ret);
    return ret;
  }
  ID3DX11EffectTechnique* Effects::getMeshTechnique(MeshRenderFlags flags)
  {
    return _meshTechniques[flags];
  }
  ID3DX11EffectTechnique * Effects::getShadowMapTechnique(ShadowMapRenderFlags flags)
  {
    return _shadowTechniques[flags];
  }
  ID3DX11EffectTechnique * Effects::getCompositeTechnique(CompositeRenderFlags flags)
  {
    return _compositeTechniques[flags];
  }
  CComPtr<ID3DX11Effect> Effects::getEffect()
  {
    return _effect;
  }
  CComPtr<ID3DX11Effect> Effects::getTerrainEffect()
  {
    return _terrainEffect;
  }
  CComPtr<ID3DX11Effect> Effects::createEffectFromFile(const std::wstring & file, const CComPtr<ID3D11Device>& device)
  {
    CComPtr<ID3DBlob> compiled_shader, err;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
    auto hr = D3DCompileFromFile(file.c_str(), nullptr, nullptr, nullptr, "fx_5_0", 0, 0, &compiled_shader, &err);
    if (err) {
      MessageBox(nullptr, reinterpret_cast<char*>(err->GetBufferPointer()), nullptr, 0);
    }
    if (FAILED(hr)) {
      DXTrace(__FILEW__, __LINE__, hr, nullptr, true);
      exit(-1);
    }
    CComPtr<ID3DX11Effect> effect;
    HR(D3DX11CreateEffectFromMemory(compiled_shader->GetBufferPointer(), compiled_shader->GetBufferSize(), 0, device, &effect));
    return effect;
  }
}