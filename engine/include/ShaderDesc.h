#ifndef SHADERDESC_H
#define SHADERDESC_H

#include <memory>
#include <StackPOD.h>
#include <renderer/RenderParams.h>
#include <Flags.h>

namespace fly
{
  template<typename API>
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
      if (flags & ShaderSetupFlags::SS_P_INVERSE) {
        _setupFuncs.push_back_secure(typename API::ShaderSetup::setupPInverse);
      }
    }
    inline void setup(const GlobalShaderParams& params) const
    {
      _api.bindShader(_shader.get());
      for (const auto& f : _setupFuncs) {
        f(params, _shader.get());
      }
    }
   /* inline const std::shared_ptr<typename API::Shader>& getShader() const
    {
      return _shader;
    }*/
  private:
    std::shared_ptr<typename API::Shader> _shader;
    StackPOD<void(*)(const GlobalShaderParams&, typename API::Shader const *)> _setupFuncs;
    API & _api;
  };
}

#endif
