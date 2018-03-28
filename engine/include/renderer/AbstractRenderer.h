#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include <StaticModelRenderable.h>
#include <System.h>
#include <renderer/ProjectionParams.h>
#include <renderer/RenderParams.h>
#include <math/FlyMath.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <map>
#include <Model.h>
#include <memory>
#include <Entity.h>
#include <Camera.h>
#include <Light.h>
#include <iostream>
#include <Quadtree.h>

namespace fly
{
  template<class API>
  class AbstractRenderer : public System
  {
  public:
    AbstractRenderer() : _api()
    {
      _pp._aspectRatio = 16.f / 9.f;
      _pp._near = 0.1f;
      _pp._far = 1000.f;
      _pp._fieldOfView = glm::radians(45.f);
    }
    virtual ~AbstractRenderer() = default;
    virtual void onComponentsChanged(Entity* entity) override
    {
      auto smr = entity->getComponent<fly::StaticModelRenderable>();
      auto camera = entity->getComponent<Camera>();
      auto dl = entity->getComponent<DirectionalLight>();
      if (smr) {
        std::vector<std::shared_ptr<ModelData>> model_data_lods;
        for (const auto& lod : smr->getLods()) {
          auto it = _modelDataCache.find(lod);
          if (it == _modelDataCache.end()) {
            auto model_data = std::make_shared<ModelData>();
            std::vector<typename API::MeshDesc> mesh_descs;
            model_data->_geometryData = std::make_shared<typename API::GeometryData>(lod->getMeshes(), &_api, mesh_descs);
            int material_index = -1;
            for (unsigned i = 0; i < lod->getMeshes().size(); i++) {
              const auto& mesh = lod->getMeshes()[i];
              if (mesh->getMaterialIndex() != material_index) { // Material has changed -> append a new batch
                model_data->_batchDescs.resize(model_data->_batchDescs.size() + 1);
                model_data->_batchDescs.back()._materialIndex = mesh->getMaterialIndex();
              }
              material_index = mesh->getMaterialIndex();
              model_data->_batchDescs.back()._batchDesc.addMeshDesc(mesh_descs[i]);
            }
            model_data->_materialDesc.resize(lod->getMaterials().size());
            for (unsigned i = 0; i < lod->getMaterials().size(); i++) {
              model_data->_materialDesc[i]._diffuseTexture = createTexture(lod->getMaterials()[i].getDiffusePath());
              model_data->_materialDesc[i]._diffuseColor = lod->getMaterials()[i].getDiffuseColor();
            }
            model_data_lods.push_back(model_data);
            _modelDataCache[lod] = model_data;
          }
          else {
            model_data_lods.push_back(it->second);
          }
        }
        // _staticModelRenderables[entity] = std::shared_ptr<API::StaticModelRenderable>(new API::StaticModelRenderable({ lods, smr }));
        _staticModelRenderables[entity] = std::make_shared<StaticModelRenderable>();
        _staticModelRenderables[entity]->_modelLods = model_data_lods;
        _staticModelRenderables[entity]->_smr = smr;
        _sceneMin = minimum(_sceneMin, smr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, smr->getAABBWorld()->getMax());
      }
      else {
        _staticModelRenderables.erase(entity);
      }
      if (camera) {
        _camera = camera;
      }
      if (dl) {
        _directionalLight = dl;
      }
    }
    virtual void update(float time, float delta_time) override
    {
      if (_quadtree == nullptr) {
        buildQuadtree();
      }
      _api.clearRendertargetColor(Vec4f({ 0.149f, 0.509f, 0.929f, 1.f }));
      if (_camera) {
        _rp._viewMatrix = _camera->getViewMatrix(_camera->_pos, _camera->_eulerAngles);
        _rp._VP = _rp._projectionMatrix * _rp._viewMatrix;
        _api.setViewport(_viewPortSize);
        _api.setDepthTestEnabled<true>();
        auto visible_elements = _quadtree->getVisibleElements<API::isDirectX(), false>({ _rp._VP });
        for (const auto& e : visible_elements) {
          const auto& model_data = e->_modelLods[e->_smr->selectLod(_camera->_pos)];
          _api.bindGeometryData(*model_data->_geometryData);
          for (const auto& batch : model_data->_batchDescs) {
            const auto& mat_desc = model_data->_materialDesc[batch._materialIndex];
            _api.setupMaterial(mat_desc._diffuseTexture, mat_desc._diffuseColor);
            _api.renderMeshBatch(batch._batchDesc, _rp._VP * e->_smr->getModelMatrix());
          }
        }
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      _pp._aspectRatio = _viewPortSize[0] / _viewPortSize[1];
      _rp._projectionMatrix = _api.getZNearMapping() == ZNearMapping::ZERO ?
        glm::perspectiveRH_ZO(_pp._fieldOfView, _pp._aspectRatio, _pp._near, _pp._far) :
        glm::perspectiveRH_NO(_pp._fieldOfView, _pp._aspectRatio, _pp._near, _pp._far);
    }
  private:
    API _api;
    ProjectionParams _pp;
    RenderParams _rp;
    Vec2f _viewPortSize;
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    Vec3f _sceneMin = Vec3f(std::numeric_limits<float>::max());
    Vec3f _sceneMax = Vec3f(std::numeric_limits<float>::lowest());

    /**
    * Meshes that share the same material are rendered in a single batch, no need to expensively switch materials between them.
    */
    struct BatchDesc
    {
      typename API::BatchDesc _batchDesc;
      int _materialIndex;
    };
    struct MaterialDesc
    {
      std::shared_ptr<typename API::Texture> _diffuseTexture;
      std::shared_ptr<typename API::Texture> _normalTexture;
      std::shared_ptr<typename API::Texture> _alphaTexture;
      Vec3f _diffuseColor;
      Vec3f _specularColor;
      Vec3f _ambientColor;
    };
    struct ModelData
    {
      std::shared_ptr<typename API::GeometryData> _geometryData;
      std::vector<BatchDesc> _batchDescs;
      std::vector<MaterialDesc> _materialDesc;
    };
    struct StaticModelRenderable
    {
      std::vector<std::shared_ptr<ModelData>> _modelLods;
      std::shared_ptr<fly::StaticModelRenderable> _smr;
      inline AABB* getAABBWorld() const { return _smr->getAABBWorld().get(); }
    };

    std::map<std::shared_ptr<Model>, std::shared_ptr<ModelData>> _modelDataCache;
    std::map<std::string, std::shared_ptr<typename API::Texture>> _textureCache;
    std::map<Entity*, std::shared_ptr<StaticModelRenderable>> _staticModelRenderables;
    std::unique_ptr<Quadtree<StaticModelRenderable>> _quadtree;

    void buildQuadtree()
    {
      _quadtree = std::make_unique<Quadtree<StaticModelRenderable>>(Vec2f({ _sceneMin[0], _sceneMin[2] }), Vec2f({ _sceneMax[0], _sceneMax[2] }));
      for (const auto& e : _staticModelRenderables) {
        _quadtree->insert(e.second);
      }
    }
    std::shared_ptr<typename API::Texture> createTexture(const std::string& path)
    {
      auto it = _textureCache.find(path);
      if (it != _textureCache.end()) {
        return it->second;
      }
      auto tex = _api.createTexture(path);
      if (tex) {
        _textureCache[path] = tex;
      }
      return tex;
    }
  };
}

#endif
