#ifndef MATERIALDESC_H
#define MATERIALDESC_H

#include <memory>
#include <Material.h>
#include <GraphicsSettings.h>
#include <Flags.h>
#include <SoftwareCache.h>
#include <StackPOD.h>
//#include <renderer/MeshRenderables.h>

namespace fly
{
  template<typename API>
  class MaterialDesc
  {
  public:
    MaterialDesc(const std::shared_ptr<Material>& material, API* api, const GraphicsSettings& settings, SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&>& texture_cache) :
      _material(material),
      _activeShader(api->getActiveShader()),
      _diffuseMap(texture_cache.getOrCreate(material->getDiffusePath(), material->getDiffusePath())),
      _normalMap(texture_cache.getOrCreate(material->getNormalPath(), material->getNormalPath())),
      _alphaMap(texture_cache.getOrCreate(material->getOpacityPath(), material->getOpacityPath())),
      _heightMap(texture_cache.getOrCreate(material->getHeightPath(), material->getHeightPath()))
    {
      create(api, settings);
    }
    void create(API* api, const GraphicsSettings& settings)
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
      auto fragment_source = api->getShaderGenerator().createMeshFragmentShaderSource(flag, settings);
      auto vertex_source = api->getShaderGenerator().createMeshVertexShaderSource(flag, settings);
      unsigned ss_flags = ShaderSetupFlags::SS_LIGHTING | ShaderSetupFlags::SS_VP;
      if (settings.gammaEnabled()) {
        ss_flags |= ShaderSetupFlags::SS_GAMMA;
      }
      if (settings.getShadows() || settings.getShadowsPCF()) {
        ss_flags |= ShaderSetupFlags::SS_SHADOWS;
      }
      _meshShaderDesc = api->createShaderDesc(api->createShader(vertex_source, fragment_source), ss_flags);
      _meshShaderDescReflective = settings.getScreenSpaceReflections() ? api->createShaderDesc(api->createShader(vertex_source, api->getShaderGenerator().createMeshFragmentShaderSource(flag | FLAG::MR_REFLECTIVE, settings)), ss_flags) : _meshShaderDesc;
      _meshShaderDescWind = settings.getWindAnimations() ? api->createShaderDesc(api->createShader(api->getShaderGenerator().createMeshVertexShaderSource(flag | FLAG::MR_WIND, settings), fragment_source), ss_flags | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME) : _meshShaderDesc;
      _meshShaderDescDepth = api->createShaderDesc(api->createShader(api->getShaderGenerator().createMeshVertexShaderDepthSource(flag, settings), api->getShaderGenerator().createMeshFragmentShaderDepthSource(flag, settings)), ShaderSetupFlags::SS_VP);
      _meshShaderDescWindDepth = settings.getWindAnimations() ? api->createShaderDesc(api->createShader(api->getShaderGenerator().createMeshVertexShaderDepthSource(flag | FLAG::MR_WIND, settings), api->getShaderGenerator().createMeshFragmentShaderDepthSource(flag | FLAG::MR_WIND, settings)), ShaderSetupFlags::SS_VP | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME) : _meshShaderDescDepth;
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
    inline const std::shared_ptr<typename API::ShaderDesc>& getMeshShaderDesc() const
    {
      return _meshShaderDesc;
    }
    inline const std::shared_ptr<typename API::ShaderDesc>& getMeshShaderDescWind() const
    {
      return _meshShaderDescWind;
    }
    inline const std::shared_ptr<typename API::ShaderDesc>& getMeshShaderDescReflective() const
    {
      return _meshShaderDescReflective;
    }
    inline const std::shared_ptr<typename API::ShaderDesc>& getMeshShaderDescDepth() const
    {
      return _meshShaderDescDepth;
    }
    inline const std::shared_ptr<typename API::ShaderDesc>& getMeshShaderDescDepthWind() const
    {
      return _meshShaderDescWindDepth;
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
    std::shared_ptr<typename API::ShaderDesc> _meshShaderDesc;
    std::shared_ptr<typename API::ShaderDesc> _meshShaderDescWind;
    std::shared_ptr<typename API::ShaderDesc> _meshShaderDescDepth;
    std::shared_ptr<typename API::ShaderDesc> _meshShaderDescWindDepth;
    std::shared_ptr<typename API::ShaderDesc> _meshShaderDescReflective;
    std::shared_ptr<typename API::Texture> const _diffuseMap;
    std::shared_ptr<typename API::Texture> const _normalMap;
    std::shared_ptr<typename API::Texture> const _alphaMap;
    std::shared_ptr<typename API::Texture> const _heightMap;
//    StackPOD<MeshRenderable<API>*, 1024> _renderables;
  };
}

#endif
