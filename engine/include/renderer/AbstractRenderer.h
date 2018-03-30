#ifndef ABSTRACTRENDERER_H
#define ABSTRACTRENDERER_H

#include <StaticMeshRenderable.h>
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
#include <Settings.h>

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
      auto camera = entity->getComponent<Camera>();
      auto dl = entity->getComponent<DirectionalLight>();
      auto mr = entity->getComponent<fly::StaticMeshRenderable>();
      if (mr) {
        auto mesh_renderable = std::make_shared<StaticMeshRenderable>();
        mesh_renderable->_materialDesc = _api.createMaterial(mr->getMaterial());
        mesh_renderable->_meshData = _meshGeometryStorage.addMesh(mr->getMesh());
        mesh_renderable->_smr = mr;
        _staticMeshRenderables[entity] = mesh_renderable;
        _sceneMin = minimum(_sceneMin, mr->getAABBWorld()->getMin());
        _sceneMax = maximum(_sceneMax, mr->getAABBWorld()->getMax());
      }
      else {
        _staticMeshRenderables.erase(entity);
        // TODO: erase the renderable from the quadtree as well
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
      if (_camera && _directionalLight) {
        _rp._viewMatrix = _camera->getViewMatrix(_camera->_pos, _camera->_eulerAngles);
        _rp._VP = _rp._projectionMatrix * _rp._viewMatrix;
        _api.setViewport(_viewPortSize);
        _api.setDepthTestEnabled<true>();
        Vec3f light_pos_view = (_rp._viewMatrix * Vec4f(_directionalLight->_pos, 1.f)).xyz();
        _meshGeometryStorage.bind();
        auto visible_elements = _quadtree->getVisibleElements<API::isDirectX(), false>({ _rp._VP });
        if (_settings._dlSortMode == DisplayListSortMode::SHADER_AND_MATERIAL) {
          std::map<std::shared_ptr<typename API::MaterialDesc::ShaderProgram>, std::map<std::shared_ptr<typename API::MaterialDesc>, std::vector<StaticMeshRenderable*>>> display_list;
          for (const auto& e : visible_elements) {
            display_list[e->_materialDesc->getShader()][e->_materialDesc].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupShader(e.first, light_pos_view, _rp._projectionMatrix);
            for (const auto& e1 : e.second) {
              _api.setupMaterial(*e1.first);
              for (const auto& smr : e1.second) {
                _api.renderMesh(smr->_meshData, _rp._viewMatrix * smr->_smr->getModelMatrix());
              }
            }
          }
        }
        else {
          std::map<std::shared_ptr<typename API::MaterialDesc>, std::vector<StaticMeshRenderable*>> display_list;
          for (const auto& e : visible_elements) {
            display_list[e->_materialDesc].push_back(e);
          }
          for (const auto& e : display_list) {
            _api.setupMaterial(*e.first, light_pos_view, _rp._projectionMatrix);
            for (const auto& smr : e.second) {
              _api.renderMesh(smr->_meshData, _rp._viewMatrix * smr->_smr->getModelMatrix());
            }
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
    Settings _settings;

    // Wrapper for StaticMeshRenderable
    struct StaticMeshRenderable
    {
      std::shared_ptr<typename API::MaterialDesc> _materialDesc;
      typename API::MeshGeometryStorage::MeshData _meshData;
      std::shared_ptr<fly::StaticMeshRenderable> _smr;
      inline AABB* getAABBWorld() const { return _smr->getAABBWorld(); }
    };
    typename API::MeshGeometryStorage _meshGeometryStorage;
    std::map<Entity*, std::shared_ptr<StaticMeshRenderable>> _staticMeshRenderables;
    std::unique_ptr<Quadtree<StaticMeshRenderable>> _quadtree;

    void buildQuadtree()
    {
      _quadtree = std::make_unique<Quadtree<StaticMeshRenderable>>(Vec2f({ _sceneMin[0], _sceneMin[2] }), Vec2f({ _sceneMax[0], _sceneMax[2] }));
      for (const auto& e : _staticMeshRenderables) {
        _quadtree->insert(e.second.get());
      }
    }
  };
}

#endif
