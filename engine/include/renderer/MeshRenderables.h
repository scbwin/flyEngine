#ifndef MESHRENDERABLES_H
#define MESHRENDERABLES_H

#include <memory>
#include <functional>
#include <GraphicsSettings.h>
#include <MaterialDesc.h>
#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <Material.h>
#include <AABB.h>
#include <StaticInstancedMeshRenderable.h>
#include <Camera.h>

namespace fly
{
 // class AABB;

  template<typename API>
  struct MeshRenderable : public GraphicsSettings::Listener
  {
    std::shared_ptr<MaterialDesc<API>> const _materialDesc;
    typename API::MeshGeometryStorage::MeshData const _meshData;
    ShaderDesc<API> const * _shaderDesc;
    ShaderDesc<API> const * _shaderDescDepth;
    API const & _api;
    MeshRenderable(const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api) :
      _materialDesc(material_desc), _meshData(mesh_data), _api(api)
    {}
    virtual AABB const * getAABBWorld() const = 0;
    virtual void renderDepth() = 0;
    virtual void render() = 0;
    // Returns true if the renderable is visible and therefore should be rendered
    virtual bool cull(const Camera& camera)
    {
      return isLargeEnough(camera);
    }
    virtual bool cullAndIntersect(const Camera& camera)
    {
      return cull(camera) && camera.intersectFrustumAABB(*getAABBWorld()) != IntersectionResult::OUTSIDE;
    }
    virtual void fetchShaderDescs()
    {
      _shaderDesc = _materialDesc->getMeshShaderDesc().get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepth().get();
    }
    virtual void normalMappingChanged(GraphicsSettings const * gs) override
    {
      fetchShaderDescs();
    };
    virtual void shadowsChanged(GraphicsSettings const * gs) override
    {
      fetchShaderDescs();
    };
    virtual void shadowMapSizeChanged(GraphicsSettings const * gs) override {};
    virtual void depthOfFieldChanged(GraphicsSettings const * gs) override {};
    virtual void compositingChanged(GraphicsSettings const * gs) override {};
    virtual void windAnimationsChanged(GraphicsSettings const * gs) override
    {
      fetchShaderDescs();
    };
    virtual void anisotropyChanged(GraphicsSettings const * gs) override {};
    virtual void cameraLerpingChanged(GraphicsSettings const * gs) override {};
    virtual void gammaChanged(GraphicsSettings const * gs) override
    {
      fetchShaderDescs();
    };
    virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) override
    {
      fetchShaderDescs();
    };
    virtual float getLargestElementAABBSize() const
    {
      return getAABBWorld()->size2();
    }
    virtual bool isLargeEnough(const Camera& camera) const
    {
      return getAABBWorld()->isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold());
    }
  };
  template<typename API>
  struct SkydomeRenderableWrapper : public MeshRenderable<API>
  {
    SkydomeRenderableWrapper(const typename API::MeshGeometryStorage::MeshData& mesh_data, ShaderDesc<API>* shader_desc, API const & api) :
      MeshRenderable(nullptr, mesh_data, api)
    {
      _shaderDesc = shader_desc;
    }
    virtual AABB const * getAABBWorld() const
    {
      return nullptr;
    }
    virtual void render()
    {
      _api.renderMesh(_meshData);
    }
    virtual void renderDepth()
    {}
  };
  template<typename API>
  struct DynamicMeshRenderableWrapper : public MeshRenderable<API>
  {
    DynamicMeshRenderableWrapper(const std::shared_ptr<DynamicMeshRenderable>& dmr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api) :
      MeshRenderable(material_desc, mesh_data, api),
      _dmr(dmr)
    {
    }
    std::shared_ptr<DynamicMeshRenderable> _dmr;
    virtual void render() override
    {
      const auto& model_matrix = _dmr->getModelMatrix();
      const auto& model_matrix_inverse = _dmr->getModelMatrixInverse();
      _api.renderMesh(_meshData, model_matrix, model_matrix_inverse);
    }
    virtual void renderDepth() override
    {
      _api.renderMesh(_meshData, _dmr->getModelMatrix());
    }
    virtual AABB const * getAABBWorld() const override final { return _dmr->getAABBWorld(); }
  };
  template<typename API>
  struct DynamicMeshRenderableReflectiveWrapper : public DynamicMeshRenderableWrapper<API>
  {
    DynamicMeshRenderableReflectiveWrapper(const std::shared_ptr<DynamicMeshRenderable>& dmr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api, const Mat3f& view_matrix_inverse) :
      DynamicMeshRenderableWrapper(dmr, material_desc, mesh_data, api),
      _viewMatrixInverse(view_matrix_inverse)
    {
    }
    virtual void fetchShaderDescs() override
    {
      _shaderDesc = _materialDesc->getMeshShaderDescReflective().get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepth().get();
    }
    std::function<void()> _renderFunc;
    Mat3f const & _viewMatrixInverse;
    virtual void render() override
    {
      _renderFunc();
    }
    virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) override
    {
      if (gs->getScreenSpaceReflections()) {
        _renderFunc = [this]() {
          const auto& model_matrix = _dmr->getModelMatrix();
          const auto& model_matrix_inverse = _dmr->getModelMatrixInverse();
          _api.renderMesh(_meshData, model_matrix, model_matrix_inverse, model_matrix_inverse * _viewMatrixInverse);
        };
      }
      else {
        _renderFunc = [this]() {
          DynamicMeshRenderableWrapper::render();
        };
      }
    };
  };
  template<typename API>
  struct StaticMeshRenderableWrapper : public MeshRenderable<API>
  {
   // StaticMeshRenderable const * _smr;
    AABB _aabb;
    Mat4f _modelMatrix;
    Mat3f _modelMatrixInverse;
    StaticMeshRenderableWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api) :
      MeshRenderable(material_desc, mesh_data, api),
    //  _smr(smr.get()),
      _aabb(*smr->getAABBWorld()),
      _modelMatrix(smr->getModelMatrix()),
      _modelMatrixInverse(smr->getModelMatrixInverse())
    {
    }
    virtual ~StaticMeshRenderableWrapper() = default;
    virtual void render() override
    {
      _api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse);
    }
    virtual void renderDepth() override
    {
      _api.renderMesh(_meshData, _modelMatrix);
    }
    virtual AABB const * getAABBWorld() const override final { return &_aabb; }
  };
  template<typename API>
  struct StaticMeshRenderableReflectiveWrapper : public StaticMeshRenderableWrapper<API>
  {
    StaticMeshRenderableReflectiveWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api, Mat3f const & view_matrix_inverse) :
      StaticMeshRenderableWrapper(smr, material_desc, mesh_data, api),
      _viewMatrixInverse(view_matrix_inverse)
    {
    }
    virtual void fetchShaderDescs() override
    {
      _shaderDesc = _materialDesc->getMeshShaderDescReflective().get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepth().get();
    }
    virtual ~StaticMeshRenderableReflectiveWrapper() = default;
    std::function<void()> _renderFunc;
    Mat3f const & _viewMatrixInverse;
    virtual void render() override
    {
      _renderFunc();
    }
    virtual void screenSpaceReflectionsChanged(GraphicsSettings const * gs) override
    {
      if (gs->getScreenSpaceReflections()) {
        _renderFunc = [this]() {
          _api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse, _modelMatrixInverse * _viewMatrixInverse);
        };
      }
      else {
        _renderFunc = [this]() {
          StaticMeshRenderableWrapper<API>::render();
        };
      }
      fetchShaderDescs();
    };
  };
  template<typename API>
  struct StaticMeshRenderableWindWrapper : public StaticMeshRenderableWrapper<API>
  {
    StaticMeshRenderableWindWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api) :
      StaticMeshRenderableWrapper(smr, material_desc, mesh_data, api),
      _windParamsLocal(smr->getWindParams())
    {
    }
    std::function<void()> _renderFunc;
    std::function<void()> _renderFuncDepth;
    WindParamsLocal _windParamsLocal;
    virtual ~StaticMeshRenderableWindWrapper() = default;
    virtual void fetchShaderDescs() override
    {
      _shaderDesc = _materialDesc->getMeshShaderDescWind().get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepthWind().get();

    //  std::cout << _materialDesc->getMaterial()->getDiffusePath() << std::endl;
    }
    virtual void render() override
    {
      _renderFunc();
    }
    virtual void renderDepth() override
    {
      _renderFuncDepth();
    }
    virtual void windAnimationsChanged(GraphicsSettings const * gs) override
    {
      if (gs->getWindAnimations()) {
        _renderFunc = [this]() {
          _api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse, _windParamsLocal, _aabb);
        };
        _renderFuncDepth = [this]() {
          _api.renderMesh(_meshData, _modelMatrix, _windParamsLocal, _aabb);
        };
      }
      else {
        _renderFunc = [this]() {
          StaticMeshRenderableWrapper<API>::render();
        };
        _renderFuncDepth = [this]() {
          StaticMeshRenderableWrapper<API>::renderDepth();
        };
      }
      fetchShaderDescs();
    };
  };
  template<typename API>
  struct StaticInstancedMeshRenderableWrapper : public MeshRenderable<API>
  {
    StaticInstancedMeshRenderable* _simr;
    std::shared_ptr<Camera> const & _camera;
    std::vector<typename API::IndirectInfo> _indirectInfo;
    typename API::StorageBuffer _aabbBuffer;
    typename API::StorageBuffer _visibleInstances;
    typename API::StorageBuffer _instanceData;
    typename API::IndirectBuffer _indirectBuffer;
    AABB _aabb;
    float _largestAABBSize;
    unsigned _numInstances;
    StaticInstancedMeshRenderableWrapper(const std::shared_ptr<StaticInstancedMeshRenderable>& simr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const std::vector<typename API::MeshGeometryStorage::MeshData>& mesh_data, API const & api, std::shared_ptr<Camera> const & camera ) :
      MeshRenderable(material_desc, mesh_data[0], api),
      _simr(simr.get()),
      _camera(camera),
      _visibleInstances(api.createStorageBuffer<unsigned>(nullptr, simr->getInstanceData().size() * mesh_data.size())),
      _instanceData(api.createStorageBuffer<StaticInstancedMeshRenderable::InstanceData>(simr->getInstanceData().data(), simr->getInstanceData().size())),
      _indirectInfo(api.indirectFromMeshData(mesh_data)),
      _indirectBuffer(api.createIndirectBuffer(_indirectInfo)),
      _aabb(*simr->getAABBWorld()),
      _largestAABBSize(simr->getLargestAABBSize()),
      _numInstances(static_cast<unsigned>(simr->getInstanceData().size()))
    {
      StackPOD<Vec4f> bounds;
      bounds.reserve(simr->getAABBsWorld().size() * 2u);
      for (const auto& aabb : simr->getAABBsWorld()) {
        bounds.push_back(Vec4f(aabb.getMin(), 1.f));
        bounds.push_back(Vec4f(aabb.getMax(), 1.f));
      }
      _aabbBuffer = std::move(api.createStorageBuffer<Vec4f>(bounds.begin(), bounds.size()));
    }
    virtual ~StaticInstancedMeshRenderableWrapper() = default;
   /* void cullInstances()
    {
      _api.cullInstances(_aabbBuffer, _numInstances, _visibleInstances, _indirectBuffer,
        _indirectInfo, _simr->getLodMultiplier(), _camera->getDetailCullingThreshold());
    }*/
    virtual void render() override
    {
      _api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual void renderDepth() override
    {
      _api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual AABB const * getAABBWorld() const override final { return &_aabb; }
    virtual void fetchShaderDescs() override
    {
      _shaderDesc = _materialDesc->getMeshShaderDescInstanced().get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepthInstanced().get();
    }
    virtual float getLargestElementAABBSize() const override
    {
      return _largestAABBSize;
    }
    virtual bool isLargeEnough(const Camera& camera) const override final
    {
      return _aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize);
    }
    virtual bool cull(const Camera& camera)
    {
      if (isLargeEnough(camera)) {
        _api.cullInstances(_aabbBuffer, _numInstances, _visibleInstances, _indirectBuffer,
          _indirectInfo, _simr->getLodMultiplier(), _camera->getDetailCullingThreshold());
        return true;
      }
      return false;
    }
    virtual bool cullAndIntersect(const Camera& camera)
    {
      return camera.intersectFrustumAABB(_aabb) != IntersectionResult::OUTSIDE && cull(camera);
    }
  };
}

#endif // !MESHRENDERABLES_H
