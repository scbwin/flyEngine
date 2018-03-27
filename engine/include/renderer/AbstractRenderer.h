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
      auto smr = entity->getComponent<StaticModelRenderable>();
      auto camera = entity->getComponent<Camera>();
      auto dl = entity->getComponent<DirectionalLight>();
      if (smr) {
        std::vector<std::shared_ptr<API::ModelData>> lods;
        for (const auto& lod : smr->getLods()) {
          auto it = _modelDataCache.find(lod);
          if (it == _modelDataCache.end()) {
            lods.push_back(std::make_shared<API::ModelData>(lod, &_api));
            _modelDataCache[lod] = lods.back();
          }
          else {
            lods.push_back(it->second);
          }
        }
        _staticModelRenderables[entity] = std::shared_ptr<API::StaticModelRenderable>(new API::StaticModelRenderable({ lods, smr }));
        _sceneMin = minimum(_sceneMin, smr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, smr->getAABBWorld()->getMax());
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
        _api.setDepthEnabled<true>();
        auto visible_elements = _quadtree->getVisibleElements<API::isDirectX(), false>({ _rp._VP });
        for (const auto& e : visible_elements) {
          auto lod = e->_smr->selectLod(_camera->_pos);
          _api.renderModel(*e, _rp._VP * e->_smr->getModelMatrix(), lod);
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
    std::map<std::shared_ptr<Model>, std::shared_ptr<typename API::ModelData>> _modelDataCache;
    std::map<Entity*, std::shared_ptr<typename API::StaticModelRenderable>> _staticModelRenderables;
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
    Vec3f _sceneMin = Vec3f(std::numeric_limits<float>::max());
    Vec3f _sceneMax = Vec3f(std::numeric_limits<float>::lowest());
    std::unique_ptr<Quadtree<typename API::StaticModelRenderable>> _quadtree;

    void buildQuadtree()
    {
      _quadtree = std::make_unique<Quadtree<typename API::StaticModelRenderable>>(Vec2f({ _sceneMin[0], _sceneMin[2] }), Vec2f({ _sceneMax[0], _sceneMax[2] }));
      for (const auto& e : _staticModelRenderables) {
        _quadtree->insert(e.second);
      }
    }
  };
}

#endif
