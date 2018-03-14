#ifndef RENDERINGSYSTEMDX11_H
#define RENDERINGSYSTEMDX11_H

#include <System.h>
#include <glm/glm.hpp>
#include <d3d11.h>
#include <atlbase.h>
#include <Model.h>
#include <Transform.h>
#include <Entity.h>
#include <map>
#include <Camera.h>
#include <d3dx11effect.h>
#include <DirectXMath.h>
#include <Light.h>
#include <SpriteFont.h>
#include <DirectXTex/DirectXTex.h>
#include <glm/gtc/quaternion.hpp>
#include <dx11/Effects.h>
#include <physics/ParticleSystem.h>
#include <set>
#include <Billboard.h>
#include <CommonStates.h>
#include <dx11/DX11States.h>
#include <TerrainNew.h>
#include <GeometryGenerator.h>
#include <math/FlyMath.h>

namespace fly
{
  class ProceduralTerrainRenderable;

  class RenderingSystemDX11 : public System
  {
  public:
    RenderingSystemDX11(HWND window);
    virtual ~RenderingSystemDX11();
    virtual void onComponentsChanged(Entity* entity) override;
    virtual void update(float time, float delta_time) override;
    void onResize(const Vec2u& size);
    void setSSRBlendWeight(float alpha);
    float getSSRBlendWeight() const;
    void initEffects();
    void present() const;
    CComPtr<ID3D11Device> getDevice() const;
    CComPtr<ID3D11DeviceContext> getContext() const;
    CComPtr<IDXGIAdapter> getAdapter() const;
    struct Settings
    {
      bool _lensflareEnabled;
      bool _depthOfFieldEnabled;
      bool _motionBlurEnabled;
      bool _vsync;
      bool _ssrEnabled;
      bool _lightVolumesEnabled;
      bool _wireframe;
      Vec3f _depthOfFieldDistances;
      Vec3f _skyColor;
      float _brightScale;
      float _brightBias;
      float _exposure;
      unsigned _lensflareLevels;
    };
    Settings _settings;
    void setSettings(const Settings& settings);
    Settings getSettings() const;

  private:
    using RtvPtr = CComPtr<ID3D11RenderTargetView>;
    using SrvPtr = CComPtr<ID3D11ShaderResourceView>;
    using DsvPtr = CComPtr<ID3D11DepthStencilView>;
    // Render to texture
    struct RTT
    {
      RTT(const Vec2u& size, RenderingSystemDX11* rs, unsigned mip_levels = 1, bool one_channel = false);
      RtvPtr _rtv;
      SrvPtr _srv;
      D3D11_VIEWPORT _viewport;
    };
    using PingPongBuffer = std::array<std::shared_ptr<RTT>, 2>;
    struct ShadowMap
    {
      ShadowMap(RenderingSystemDX11* rs);
      int _size = 1024;
      unsigned int _numCascades = 2;
      SrvPtr _srv;
      std::vector<DsvPtr> _dsv; // For each slice
      D3D11_VIEWPORT _viewPort;
      /*const unsigned _mips = 3;
      std::vector<std::vector<SrvPtr>> _minMaxSrvs; // For each slice and mip level
      std::vector<std::vector<RtvPtr>> _minMaxRtvs;
      std::vector<std::vector<D3D11_VIEWPORT>> _minMaxViewports;*/
    };
    struct DL
    {
      std::shared_ptr<Transform> _transform;
      std::shared_ptr<DirectionalLight> _dl;
      std::unique_ptr<ShadowMap> _shadowMap;
    };
    struct DownsampleChain
    {
      DownsampleChain(RenderingSystemDX11* rs, unsigned int levels, const Vec2u& divisor = Vec2u(2u));
      std::vector<PingPongBuffer> _buffers;
    };
    struct ModelData
    {
      ModelData(const std::shared_ptr<Model>& model, RenderingSystemDX11* rs);
      CComPtr<ID3D11Buffer> _vertexBuffer;
      CComPtr<ID3D11Buffer> _indexBuffer;
      struct MeshDesc
      {
        unsigned _indexOffset;
        unsigned _baseVertex;
        unsigned _numIndices;
        unsigned _materialIndex;
        Mesh* _mesh;
      };
      std::vector<MeshDesc> _meshDesc;
      struct MaterialDesc
      {
        SrvPtr _diffuseSrv;
        SrvPtr _opacitySrv;
        SrvPtr _normalSrv;
        Vec3f _diffuseColor;
        ID3DX11EffectPass* _pass;
        ID3DX11EffectPass* _shadowMapPass;
      };
      std::vector<MaterialDesc> _materialDesc;
    };
    struct DX11StaticModelRenderable
    {
      DX11StaticModelRenderable(const std::shared_ptr<Model>& model, const std::shared_ptr<Transform>& transform, RenderingSystemDX11* rs);
      std::shared_ptr<Model> _model;
      std::unique_ptr<ModelData> _modelData;
      Mat4f _modelMatrix;
      Mat4f _mvInverse;
      Mat4f _mvp;
    };
    struct DX11ProceduralTerrainRenderable
    {
      DX11ProceduralTerrainRenderable(const std::shared_ptr<TerrainNew>& terrain_new, const std::shared_ptr<ProceduralTerrainRenderable>& ptr, RenderingSystemDX11* rs);
      std::shared_ptr<TerrainNew> _terrain;
      std::shared_ptr<ProceduralTerrainRenderable> _ptr;
      CComPtr<ID3D11Buffer> _vertexBuffer;
      CComPtr<ID3D11Buffer> _indexBuffer;
      std::map<unsigned, std::map<unsigned, GeometryGenerator::IndexBufferInfo>> _indexBufferInfo;
      std::vector<SrvPtr> _terrainSrvs;
      std::vector<SrvPtr> _terrainNormalSrvs;
      SrvPtr _noiseSrv;
    };
    std::unique_ptr<DX11ProceduralTerrainRenderable> _proceduralTerrainRenderable;

    struct DX11SkyboxRenderable
    {
      DX11SkyboxRenderable(const std::shared_ptr<Model>& model, RenderingSystemDX11* rs);
      std::shared_ptr<Model> _model;
      std::unique_ptr<ModelData> _modelData;
    };
    std::unique_ptr<DX11SkyboxRenderable> _skyboxRenderable;

    struct ParticleModelRenderable
    {
      std::shared_ptr<DX11StaticModelRenderable> _modelRenderable;
      std::shared_ptr<ParticleSystem> _particleSystem;
    };
    struct ParticleBillboardRenderable
    {
      std::shared_ptr<Billboard> _billboard;
      std::shared_ptr<ParticleSystem> _particleSystem;
      SrvPtr _srv;
    };

    CComPtr<ID3D11Device> _device;
    CComPtr<ID3D11DeviceContext> _context;
    CComPtr<IDXGISwapChain> _swapChain;
    CComPtr<IDXGIAdapter> _dxgiAdapter;
    RtvPtr _backBufferRtv;
    DsvPtr _depthStencilView;
    SrvPtr _depthStencilSrv;
    SrvPtr _depthStencilSrvCopy;
    std::unique_ptr<RTT> _lightingBuffer;
    std::unique_ptr<RTT> _lightingBufferCopy;
    std::unique_ptr<RTT> _viewSpaceNormals;
    std::unique_ptr<RTT> _viewSpaceZ;
    CComPtr<ID3D11RasterizerState> _rastStateShadowMap;
    CComPtr<ID3D11InputLayout> _defaultInputLayout;
    D3D11_VIEWPORT _viewPort;
    std::unique_ptr<DX11States> _dx11States;
    std::unique_ptr<DirectX::CommonStates> _commonStates;
    D3D11_SHADER_RESOURCE_VIEW_DESC _dsSrvDesc = {};
    D3D11_TEXTURE2D_DESC _dsTextureDesc = {};

    std::unique_ptr<Effects> _effects;
    ID3DX11EffectPass* _compositePass;
    ID3DX11EffectPass* _brightPass;
    ID3DX11EffectPass* _gaussPassHor;
    ID3DX11EffectPass* _gaussPassVert;
    ID3DX11EffectPass* _copyPass;
    ID3DX11EffectPass* _lightSourcePass;
    ID3DX11EffectPass* _ssrPass;
    ID3DX11EffectPass* _lightParticlePass;
    ID3DX11EffectPass* _billboardPass;
    ID3DX11EffectPass* _minMaxPass;
    ID3DX11EffectPass* _minMaxPass2;
    ID3DX11EffectMatrixVariable* _fxMVInverseTranspose;
    ID3DX11EffectMatrixVariable* _fxV;
    ID3DX11EffectMatrixVariable* _fxVInverse;
    ID3DX11EffectMatrixVariable* _fxP;
    ID3DX11EffectMatrixVariable* _fxPInverse;
    ID3DX11EffectMatrixVariable* _fxVP;
    ID3DX11EffectMatrixVariable* _fxVPBefore;
    ID3DX11EffectMatrixVariable* _fxVPInverse;
    ID3DX11EffectMatrixVariable* _fxVPInverseVPBefore;
    ID3DX11EffectMatrixVariable* _fxLightMVP;
    ID3DX11EffectMatrixVariable* _fxLightVPs;
    ID3DX11EffectMatrixVariable* _fxMVPs;
    ID3DX11EffectMatrixVariable* _fxMVP;
    ID3DX11EffectMatrixVariable* _fxMVPTerrain;
    ID3DX11EffectScalarVariable* _fxCascDistances;
    ID3DX11EffectScalarVariable* _fxTime;
    ID3DX11EffectScalarVariable* _fxMotionBlurStrength;
    ID3DX11EffectShaderResourceVariable* _fxDiffuseTex;
    ID3DX11EffectShaderResourceVariable* _fxAlphaMap;
    ID3DX11EffectShaderResourceVariable* _fxNormalMap;
    ID3DX11EffectShaderResourceVariable* _fxLightingTex;
    ID3DX11EffectShaderResourceVariable* _fxDepthTex;
    ID3DX11EffectShaderResourceVariable* _fxShadowMap;
    ID3DX11EffectShaderResourceVariable* _fxBlurInputTex;
    ID3DX11EffectShaderResourceVariable* _fxTexToCopy;
    ID3DX11EffectShaderResourceVariable* _fxLensflareTexture;
    ID3DX11EffectShaderResourceVariable* _fxDofTexture;
    ID3DX11EffectShaderResourceVariable* _fxVsNormalsTexture;
    ID3DX11EffectShaderResourceVariable* _fxVsZTexture;
    ID3DX11EffectShaderResourceVariable* _fxMinMaxTexture;
    ID3DX11EffectShaderResourceVariable* _fxMinMaxTexture2;
    ID3DX11EffectVectorVariable* _fxDiffuseColor;
    ID3DX11EffectVectorVariable* _fxLightPosView;
    ID3DX11EffectVectorVariable* _fxLightColor;
    ID3DX11EffectScalarVariable* _fxWindPivotMinY;
    ID3DX11EffectScalarVariable* _fxWindPivotMaxY;
    ID3DX11EffectVectorVariable* _fxTexelSize;
    ID3DX11EffectVectorVariable* _fxQuadScale;
    ID3DX11EffectVectorVariable* _fxQuadPos;
    ID3DX11EffectScalarVariable* _fxCascadeIndex;
    ID3DX11EffectScalarVariable* _fxWindStrength;
    ID3DX11EffectScalarVariable* _fxWindFrequency;
    ID3DX11EffectVectorVariable* _fxBillboardPosWorld;
    ID3DX11EffectVectorVariable* _fxCamUpWorld;
    ID3DX11EffectVectorVariable* _fxCamRightWorld;
    ID3DX11EffectVectorVariable* _fxBillboardSize;
    ID3DX11EffectScalarVariable* _fxFades;
    ID3DX11EffectVectorVariable* _fxCamPosWorld;
    ID3DX11EffectScalarVariable* _fxminMaxArraySlice;
    ID3DX11EffectVectorVariable* _fxLightPosWorldTerrain;
    ID3DX11EffectShaderResourceVariable* _fxNoiseTexture;
    ID3DX11EffectShaderResourceVariable* _fxTerrainTexture;
    ID3DX11EffectShaderResourceVariable* _fxTerrainTexture2;
    ID3DX11EffectShaderResourceVariable* _fxTerrainNormals;
    ID3DX11EffectShaderResourceVariable* _fxTerrainNormals2;
    ID3DX11EffectVectorVariable* _fxCamPosWorldTerrain;
    ID3DX11EffectScalarVariable* _fxPtrFrequ;
    ID3DX11EffectScalarVariable* _fxPtrHeightScale;
    ID3DX11EffectScalarVariable* _fxPtrNumOctaves;
    ID3DX11EffectScalarVariable* _fxAmpScale;
    ID3DX11EffectScalarVariable* _fxFrequencyScale;
    ID3DX11EffectScalarVariable* _fxUvScaleDetails;
    ID3DX11EffectVectorVariable* _fxDepthOfFieldDistances;
    ID3DX11EffectScalarVariable* _fxTerrainSize;
    ID3DX11EffectScalarVariable* _fxMaxTessFactor;
    ID3DX11EffectScalarVariable* _fxMaxTessDistance;
    ID3DX11EffectVectorVariable* _fxSkycolor;
    ID3DX11EffectScalarVariable* _fxBrightScale;
    ID3DX11EffectScalarVariable* _fxBrightBias;
    ID3DX11EffectScalarVariable* _fxExposure;

    Mat4f _viewMatrix;
    Mat4f _projectionMatrix;
    Mat4f _PInverse;
    Mat4f _VP;
    Mat4f _vpInverseVPBefore;
    Mat4f _VPBefore;
    std::vector<Mat4f> _lightVP;
    Vec2u _viewportSize;
    float _aspectRatio = 1.f;
    float _fov = 45.f;
    float _near = 0.1f;
    float _far = 20000.f;

    std::shared_ptr<Camera> _camera;

    bool _reflectiveSurfacesVisible;
    Vec3f _camPos;
    glm::quat _camEulerAngles;
    const float _targetFPS = 60.f;
    float _fpsFactor = 1.f;
    const float _lerpAlpha = 0.95f;
    float _deltaTime;
    float _deltaTimeFiltered;
    float _time;
    float _ssrBlendWeight = 0.8f;
    const int _terrainTessFactor = 64;
    const unsigned _terrainLods = 5;
    glm::vec4 _backgroundColor = glm::pow(glm::vec4(95.f / 255.f, 137.0f / 255.f, 204.f / 255.f, 1.f), glm::vec4(2.2f));

    std::map<Entity*, std::shared_ptr<DX11StaticModelRenderable>> _staticModelRenderables;
    std::map<Entity*, ParticleModelRenderable> _particleModelRenderables;
    std::map<Entity*, ParticleBillboardRenderable> _particleBillboardRenderables;

    DL _directionalLight; // only 1 directional light is supported for now
    std::unique_ptr<DownsampleChain> _lensFlareChain;
    PingPongBuffer _dofBuffers;

    void initAdditionalRenderTargets();
    void prepareRender(float time, float delta_time);
    void renderShadowMaps() const;
    void renderScene();
    void renderModels();
    void renderTerrain() const;
    void renderOpaqueParticles() const;
    void renderTransparentParticles() const;
    void renderSkybox() const;
    void renderLightsources() const;
    void postProcessing() const;
    void renderLensflares() const;
    void ssr() const;
    void composite() const;
    void createTexture(LPCWSTR path, SrvPtr& srv, DirectX::TEX_FILTER_FLAGS tex_filter);
    void gaussFilter(const PingPongBuffer& buffer, const SrvPtr& input) const;
    void renderBrightPass(const RTT& rtt, const SrvPtr& srv) const;
    void copy(const SrvPtr& from, const RtvPtr& to, const D3D11_VIEWPORT& vp) const;
    void copy(const SrvPtr& from, const SrvPtr& to) const;
  };
}

#endif
