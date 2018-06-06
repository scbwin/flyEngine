#ifndef SHADERDESC_H
#define SHADERDESC_H

#include <memory>
#include <StackPOD.h>
#include <Flags.h>
#include <functional>

namespace fly
{
  template<typename API, typename BV>
  class MaterialDesc;
  
  struct GlobalShaderParams;

  /**
  * Class that wraps a single shader program and sets up the necessary uniform data once the shader is bound. 
  */
  template<typename API, typename BV>
  class ShaderDesc
  {
  public:
    ShaderDesc(const std::shared_ptr<typename API::Shader>& shader, unsigned flags, API& api) : 
      _shader(shader), 
      _api(api)
    {
      if (flags & ShaderSetupFlags::SS_VP) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupVP);
      }
      if (flags & ShaderSetupFlags::SS_LIGHTING) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupLighting);
      }
      if (flags & ShaderSetupFlags::SS_SHADOWS) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupShadows);
      }
      if (flags & ShaderSetupFlags::SS_TIME) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupTime);
      }
      if (flags & ShaderSetupFlags::SS_WIND) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupWind);
      }
      if (flags & ShaderSetupFlags::SS_GAMMA) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupGamma);
      }
      if (flags & ShaderSetupFlags::SS_WORLD_TO_LIGHT) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupWorldToLight);
      }
      if (flags & ShaderSetupFlags::SS_V_INVERSE) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupVInverse);
      }
    }
    inline void setup(const GlobalShaderParams& params) const
    {
      _api.bindShader(_shader.get());
      for (const auto& f : _setupFuncs) {
        f(params, _shader.get());
      }
    }
    inline void addMaterial(MaterialDesc<API, BV>* material)
    {
      if (_materials.find(material) == _materials.end()) {
        _materials.push_back_secure(material);
      }
    }
    inline void clearMaterials()
    {
      _materials.clear();
    }
    inline const StackPOD<MaterialDesc<API, BV>*>& getMaterials() const
    {
      return _materials;
    }
    inline bool isUsed() const
    {
      return _isUsed;
    }
    inline void setIsUsed(bool is_used)
    {
      _isUsed = is_used;
    }
  private:
    std::shared_ptr<typename API::Shader> _shader;
    StackPOD<void(*)(const GlobalShaderParams&, typename API::Shader const *)> _setupFuncs;
    API & _api;
    StackPOD<MaterialDesc<API, BV>*> _materials;
    bool _isUsed = false;
  };
}

#endif
