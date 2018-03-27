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

namespace fly
{
  template<class API>
  class AbstractRenderer : public System
  {
  public:
    AbstractRenderer() : _api()
    {
      _projectionParams._aspectRatio = 16.f / 9.f;
      _projectionParams._near = 0.1f;
      _projectionParams._far = 1000.f;
      _projectionParams._fieldOfView = glm::radians(45.f);
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
      if (_camera) {
        _renderParams._viewMatrix = _camera->getViewMatrix(_camera->_pos, _camera->_eulerAngles);
        _renderParams._VP = _renderParams._projectionMatrix * _renderParams._viewMatrix;
        _api.setViewport(_viewPortSize);
        _api.setDepthEnabled<true>();
        for (const auto& e : _staticModelRenderables) {
          if (e.second->_smr->getAABBWorld()->isVisible<API::isDirectX(), false>(_renderParams._VP)) {
            auto lod = e.second->_smr->selectLod(_camera->_pos);
            _api.renderModel(*e.second, _renderParams._VP * e.second->_smr->getModelMatrix(), lod);
          }
        }
      }
    }
    void onResize(const Vec2u& window_size)
    {
      _viewPortSize = window_size;
      _projectionParams._aspectRatio = _viewPortSize[0] / _viewPortSize[1];
      _renderParams._projectionMatrix = _api.getZNearMapping() == ZNearMapping::ZERO ?
        glm::perspectiveRH_ZO(_projectionParams._fieldOfView, _projectionParams._aspectRatio, _projectionParams._near, _projectionParams._far) :
        glm::perspectiveRH_NO(_projectionParams._fieldOfView, _projectionParams._aspectRatio, _projectionParams._near, _projectionParams._far);
    }
  private:
    API _api;
    ProjectionParams _projectionParams;
    RenderParams _renderParams;
    Vec2f _viewPortSize;
    std::map<std::shared_ptr<Model>, std::shared_ptr<typename API::ModelData>> _modelDataCache;
    std::map<Entity*, std::shared_ptr<typename API::StaticModelRenderable>> _staticModelRenderables;
    std::shared_ptr<Camera> _camera;
    std::shared_ptr<DirectionalLight> _directionalLight;
  };
}

#endif
