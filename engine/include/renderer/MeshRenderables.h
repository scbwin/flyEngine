#ifndef MESHRENDERABLES_H
#define MESHRENDERABLES_H

#include <memory>
#include <functional>
#include <GraphicsSettings.h>
#include <MaterialDesc.h>
#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>
#include <Material.h>

namespace fly
{
  class AABB;

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
    std::shared_ptr<StaticMeshRenderable> _smr;
    StaticMeshRenderableWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<MaterialDesc<API>>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data, API const & api) :
      MeshRenderable(material_desc, mesh_data, api),
      _smr(smr)
    {
    }
    virtual ~StaticMeshRenderableWrapper() = default;
    virtual void render() override
    {
      _api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse());
    }
    virtual void renderDepth() override
    {
      _api.renderMesh(_meshData, _smr->getModelMatrix());
    }
    virtual AABB const * getAABBWorld() const override final { return _smr->getAABBWorld(); }
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
          _api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getModelMatrixInverse() * _viewMatrixInverse);
        };
      }
      else {
        _renderFunc = [this]() {
          StaticMeshRenderableWrapper::render();
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
      StaticMeshRenderableWrapper(smr, material_desc, mesh_data, api)
    {
    }
    std::function<void()> _renderFunc;
    std::function<void()> _renderFuncDepth;
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
          _api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getWindParams(), *getAABBWorld());
        };
        _renderFuncDepth = [this]() {
          _api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getWindParams(), *getAABBWorld());
        };
      }
      else {
        _renderFunc = [this]() {
          StaticMeshRenderableWrapper::render();
        };
        _renderFuncDepth = [this]() {
          StaticMeshRenderableWrapper::renderDepth();
        };
      }
      fetchShaderDescs();
    };
  };
}

#endif // !MESHRENDERABLES_H