#ifndef MATERIALDESC_H
#define MATERIALDESC_H

#include <memory>
#include <Material.h>
#include <GraphicsSettings.h>
#include <Flags.h>
#include <SoftwareCache.h>
#include <StackPOD.h>
#include <ShaderDesc.h>
//#include <renderer/MeshRenderables.h>

namespace fly
{
  template<typename API>
  class MaterialDesc
  {
  public:
    MaterialDesc(const std::shared_ptr<Material>& material, API& api, const GraphicsSettings& settings,
      SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&>& texture_cache,
      SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&>& shader_desc_cache,
      SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&>& shader_cache) :
      _material(material),
      _activeShader(api.getActiveShader()),
      _diffuseMap(texture_cache.getOrCreate(material->getDiffusePath(), material->getDiffusePath())),
      _normalMap(texture_cache.getOrCreate(material->getNormalPath(), material->getNormalPath())),
      _alphaMap(texture_cache.getOrCreate(material->getOpacityPath(), material->getOpacityPath())),
      _heightMap(texture_cache.getOrCreate(material->getHeightPath(), material->getHeightPath())),
      _shaderDescCache(shader_desc_cache),
      _shaderCache(shader_cache)
    {
      create(api, settings);
    }
    void create(API& api, const GraphicsSettings& settings)
    {
      _materialSetupFuncs.clear();
      _materialSetupFuncsDepth.clear();

      using FLAG = MeshRenderFlag;
      unsigned flag = FLAG::MR_NONE;
      _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupMaterialConstants);
      if (_diffuseMap) {
        flag |= FLAG::MR_DIFFUSE_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupDiffuse);
      }
      else {
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupDiffuseColor);
      }
      if (_alphaMap) {
        flag |= FLAG::MR_ALPHA_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupAlpha);
        _materialSetupFuncsDepth.push_back_secure(typename API::MaterialSetup::setupAlpha);
      }
      if (_normalMap && settings.getNormalMapping()) {
        flag |= FLAG::MR_NORMAL_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupNormal);
      }
      if (_normalMap && _heightMap && settings.getNormalMapping() && settings.getParallaxMapping()) {
        flag |= FLAG::MR_HEIGHT_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupHeight);
        if (settings.getReliefMapping()) {
          _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupRelief);
        }
      }
      auto fragment_source = api.getShaderGenerator().createMeshFragmentShaderSource(flag, settings);
      auto vertex_source = api.getShaderGenerator().createMeshVertexShaderSource(flag, settings);
      unsigned ss_flags = ShaderSetupFlags::SS_LIGHTING | ShaderSetupFlags::SS_VP;
      if (settings.gammaEnabled()) {
        ss_flags |= ShaderSetupFlags::SS_GAMMA;
      }
      if (settings.getShadows() || settings.getShadowsPCF()) {
        ss_flags |= ShaderSetupFlags::SS_SHADOWS;
      }
      _meshShaderDesc = createShaderDesc(createShader(vertex_source, fragment_source), ss_flags, api);
      _meshShaderDescReflective = settings.getScreenSpaceReflections() ? createShaderDesc(createShader(vertex_source, api.getShaderGenerator().createMeshFragmentShaderSource(flag | FLAG::MR_REFLECTIVE, settings)), ss_flags, api) : _meshShaderDesc;
      _meshShaderDescWind = settings.getWindAnimations() ? createShaderDesc(createShader(api.getShaderGenerator().createMeshVertexShaderSource(flag | FLAG::MR_WIND, settings), fragment_source), ss_flags | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME, api) : _meshShaderDesc;
      _meshShaderDescDepth = createShaderDesc(createShader(api.getShaderGenerator().createMeshVertexShaderDepthSource(flag, settings), api.getShaderGenerator().createMeshFragmentShaderDepthSource(flag, settings)), ShaderSetupFlags::SS_VP, api);
      _meshShaderDescWindDepth = settings.getWindAnimations() ? createShaderDesc(createShader(api.getShaderGenerator().createMeshVertexShaderDepthSource(flag | FLAG::MR_WIND, settings), api.getShaderGenerator().createMeshFragmentShaderDepthSource(flag | FLAG::MR_WIND, settings)), ShaderSetupFlags::SS_VP | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME, api) : _meshShaderDescDepth;
    }
    inline void setup() const
    {
      for (const auto& f : _materialSetupFuncs) {
        f(_activeShader, *this);
      }
    }
    inline void setupDepth() const
    {
      for (const auto& f : _materialSetupFuncsDepth) {
        f(_activeShader, *this);
      }
    }
    inline const std::shared_ptr<Material>& getMaterial() const
    {
      return _material;
    }
    inline const std::shared_ptr<typename API::Texture>& diffuseMap() const
    {
      return _diffuseMap;
    }
    inline const std::shared_ptr<typename API::Texture>& normalMap() const
    {
      return _normalMap;
    }
    inline const std::shared_ptr<typename API::Texture>& alphaMap() const
    {
      return _alphaMap;
    }
    inline const std::shared_ptr<typename API::Texture>& heightMap() const
    {
      return _heightMap;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDesc() const
    {
      return _meshShaderDesc;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescWind() const
    {
      return _meshShaderDescWind;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescReflective() const
    {
      return _meshShaderDescReflective;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescDepth() const
    {
      return _meshShaderDescDepth;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescDepthWind() const
    {
      return _meshShaderDescWindDepth;
    }
    inline std::shared_ptr<ShaderDesc<API>> createShaderDesc(const std::shared_ptr<typename API::Shader>& shader, unsigned flags, API& api)
    {
      return _shaderDescCache.getOrCreate(shader, shader, flags, api);
    }
    inline std::shared_ptr<typename API::Shader> createShader(typename API::ShaderSource& vs, typename API::ShaderSource& fs, typename API::ShaderSource& gs = typename API::ShaderSource())
    {
      return _shaderCache.getOrCreate(vs._key + fs._key + gs._key, vs, fs, gs);
    }
    /* inline StackPOD<MeshRenderable<API>*, 1024>& getRenderables()
    {
    return _renderables;
    }*/
  private:
    typename API::Shader * & _activeShader;
    std::shared_ptr<Material> _material;
    StackPOD<void(*)(typename API::Shader*, const MaterialDesc&), 8> _materialSetupFuncs;
    StackPOD<void(*)(typename API::Shader*, const MaterialDesc&), 8> _materialSetupFuncsDepth;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDesc;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescWind;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescDepth;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescWindDepth;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescReflective;
    std::shared_ptr<typename API::Texture> const _diffuseMap;
    std::shared_ptr<typename API::Texture> const _normalMap;
    std::shared_ptr<typename API::Texture> const _alphaMap;
    std::shared_ptr<typename API::Texture> const _heightMap;
    SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&>& _shaderDescCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&>& _shaderCache;
    //    StackPOD<MeshRenderable<API>*, 1024> _renderables;
  };
}

#endif
