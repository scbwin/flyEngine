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
  template<typename T, typename BV>
  class IMeshRenderable;

  /**
  * Wraps a single material and generates shaders based on the material properties.
  * The setup() method takes care of sending the necessary uniform data to the GPU once the material is bound.
  * When the graphics settings change, all the shaders are recreated.
  */
  template<typename API>
  class MaterialDesc : public GraphicsSettings::Listener
  {
  public:
    MaterialDesc(const std::shared_ptr<Material>& material, API& api, const GraphicsSettings& settings,
      SoftwareCache<std::string, std::shared_ptr<typename API::Texture>, const std::string&>& texture_cache,
      SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&>& shader_desc_cache,
      SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&>& shader_cache) :
      _api(api),
      _material(material),
      _activeShader(api.getActiveShader()),
      _shaderDescCache(shader_desc_cache),
      _shaderCache(shader_cache)
    {
      for (const auto& e : material->getTexturePaths()) {
        _textures[e.first] = texture_cache.getOrCreate(e.second, e.second);
      }
      create(settings);
    }
    void create(const GraphicsSettings& settings)
    {
      _materialSetupFuncs.clear();
      _materialSetupFuncsDepth.clear();

      using FLAG = MeshRenderFlag;
      unsigned flag = FLAG::MR_NONE;
      _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupMaterialConstants);
      if (_material->hasTexture(Material::TextureKey::ALBEDO)) {
        flag |= FLAG::MR_DIFFUSE_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupDiffuse);
      }
      else {
        if (_material->getDiffuseColors().size()) {
          _diffuseColorBuffer = std::move(_api.createStorageBuffer(_material->getDiffuseColors().data(), _material->getDiffuseColors().size()));
          _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupDiffuseColors);
        }
        else {
          _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupDiffuseColor);
        }
      }
      if (_material->hasTexture(Material::TextureKey::ALPHA)) {
        flag |= FLAG::MR_ALPHA_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupAlpha);
        _materialSetupFuncsDepth.push_back_secure(typename API::MaterialSetup::setupAlpha);
      }
      if (_material->hasTexture(Material::TextureKey::NORMAL) && settings.getNormalMapping()) {
        flag |= FLAG::MR_NORMAL_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupNormal);
      }
      if (_material->hasTexture(Material::TextureKey::NORMAL) && _material->hasTexture(Material::TextureKey::HEIGHT) && settings.getNormalMapping() && settings.getParallaxMapping()) {
        flag |= FLAG::MR_HEIGHT_MAP;
        _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupHeight);
        if (settings.getReliefMapping()) {
          _materialSetupFuncs.push_back_secure(typename API::MaterialSetup::setupRelief);
        }
      }
      unsigned ss_flags = ShaderSetupFlags::SS_LIGHTING | ShaderSetupFlags::SS_VP;
      if (settings.gammaEnabled()) {
        ss_flags |= ShaderSetupFlags::SS_GAMMA;
      }
      if (settings.getShadows() || settings.getShadowsPCF()) {
        ss_flags |= ShaderSetupFlags::SS_SHADOWS;
      }
      if (settings.getScreenSpaceReflections() && _material->isReflective()) {
        flag |= FLAG::MR_REFLECTIVE;
        ss_flags |= ShaderSetupFlags::SS_V_INVERSE;
      }
      auto fragment_source = _api.getShaderGenerator().createMeshFragmentShaderSource(flag, settings);
      auto vertex_source = _api.getShaderGenerator().createMeshVertexShaderSource(flag, settings);
      auto vertex_source_instanced = _api.getShaderGenerator().createMeshVertexShaderSource(flag, settings, true);
      auto fragment_source_instanced = _api.getShaderGenerator().createMeshFragmentShaderSource(flag, settings, true);
      _meshShaderDesc = createShaderDesc(createShader(vertex_source, fragment_source), ss_flags, _api);
      _meshShaderDescDepth = createShaderDesc(createShader(_api.getShaderGenerator().createMeshVertexShaderDepthSource(flag, settings), _api.getShaderGenerator().createMeshFragmentShaderDepthSource(flag, settings)), ShaderSetupFlags::SS_VP, _api);
      _meshShaderDescInstanced = createShaderDesc(createShader(vertex_source_instanced, fragment_source_instanced), ss_flags, _api);
      _meshShaderDescDepthInstanced = createShaderDesc(createShader(_api.getShaderGenerator().createMeshVertexShaderDepthSource(flag, settings, true), _api.getShaderGenerator().createMeshFragmentShaderDepthSource(flag, settings)), ShaderSetupFlags::SS_VP, _api);
      _meshShaderDescWind = createShaderDesc(createShader(_api.getShaderGenerator().createMeshVertexShaderSource(flag | FLAG::MR_WIND, settings), fragment_source), ss_flags | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME, _api);
      _meshShaderDescDepthWind = createShaderDesc(createShader(_api.getShaderGenerator().createMeshVertexShaderDepthSource(flag | FLAG::MR_WIND, settings), _api.getShaderGenerator().createMeshFragmentShaderDepthSource(flag | FLAG::MR_WIND, settings)), ShaderSetupFlags::SS_VP | ShaderSetupFlags::SS_WIND | ShaderSetupFlags::SS_TIME, _api);
    }
    template<bool depth>
    inline void setup() const
    {
      for (const auto& f : (depth ? _materialSetupFuncsDepth : _materialSetupFuncs)) {
        f(*_activeShader, *this);
      }
    }
    inline const std::shared_ptr<Material>& getMaterial() const
    {
      return _material;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDesc() const
    {
      return _meshShaderDesc;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescInstanced() const
    {
      return _meshShaderDescInstanced;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescWind() const
    {
      return _meshShaderDescWind;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescDepth() const
    {
      return _meshShaderDescDepth;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescDepthWind() const
    {
      return _meshShaderDescDepthWind;
    }
    inline const std::shared_ptr<ShaderDesc<API>>& getMeshShaderDescDepthInstanced() const
    {
      return _meshShaderDescDepthInstanced;
    }
    inline std::shared_ptr<ShaderDesc<API>> createShaderDesc(const std::shared_ptr<typename API::Shader>& shader, unsigned flags, API& _api)
    {
      return _shaderDescCache.getOrCreate(shader, shader, flags, _api);
    }
    inline std::shared_ptr<typename API::Shader> createShader(typename API::ShaderSource& vs, typename API::ShaderSource& fs, typename API::ShaderSource& gs = typename API::ShaderSource())
    {
      return _shaderCache.getOrCreate(vs._key + fs._key + gs._key, vs, fs, gs);
    }
    inline const typename API::StorageBuffer& getDiffuseColorBuffer() const
    {
      return _diffuseColorBuffer;
    }
    virtual void normalMappingChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void shadowsChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void shadowMapSizeChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void depthOfFieldChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void compositingChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void anisotropyChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void gammaChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) override { create(*gs); }
    virtual void godRaysChanged(GraphicsSettings const * gs) override { create(*gs); }
    inline const std::shared_ptr<typename API::Texture>& getTexture(Material::TextureKey key) const
    {
      return _textures.at(key);
    }
  private:
    API & _api;
    typename API::Shader const * & _activeShader;
    std::shared_ptr<Material> const _material;
    StackPOD<void(*)(typename API::Shader const &, const MaterialDesc<API>&)> _materialSetupFuncs;
    StackPOD<void(*)(typename API::Shader const &, const MaterialDesc<API>&)> _materialSetupFuncsDepth;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDesc;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescDepth;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescWind;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescDepthWind;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescInstanced;
    std::shared_ptr<ShaderDesc<API>> _meshShaderDescDepthInstanced;
    std::map<Material::TextureKey, std::shared_ptr<typename API::Texture>> _textures;
    typename API::StorageBuffer _diffuseColorBuffer;
    SoftwareCache<std::shared_ptr<typename API::Shader>, std::shared_ptr<ShaderDesc<API>>, const std::shared_ptr<typename API::Shader>&, unsigned, API&>& _shaderDescCache;
    SoftwareCache<std::string, std::shared_ptr<typename API::Shader>, typename API::ShaderSource&, typename API::ShaderSource&, typename API::ShaderSource&>& _shaderCache;
  };
}

#endif
