#ifndef MESHRENDERABLES_H
#define MESHRENDERABLES_H

#include <memory>
#include <StaticMeshRenderable.h>
#include <DynamicMeshRenderable.h>

namespace fly
{
  class AABB;

  template<typename API>
  struct MeshRenderable
  {
    std::shared_ptr<typename API::MaterialDesc> _materialDesc;
    typename API::MeshGeometryStorage::MeshData _meshData;
    typename API::ShaderDesc* _shaderDesc;
    typename API::ShaderDesc* _shaderDescDepth;
    virtual void render(const API& api) = 0;
    virtual void renderDepth(const API& api) = 0;
    MeshRenderable(const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
      _materialDesc(material_desc), _meshData(mesh_data)
    {}
    virtual void fetchShaderDescs()
    {
      _shaderDesc = _materialDesc->getMeshShaderDesc(false).get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepth(false).get();
    }
    virtual AABB* getAABBWorld() const = 0;
  };
  template<typename API>
  struct SkydomeRenderableWrapper : public MeshRenderable<API>
  {
    SkydomeRenderableWrapper(const typename API::MeshGeometryStorage::MeshData& mesh_data, typename API::ShaderDesc* shader_desc) :
      MeshRenderable(nullptr, mesh_data)
    {
      _shaderDesc = shader_desc;
    }
    virtual AABB* getAABBWorld() const
    {
      return nullptr;
    }
    virtual void render(const API& api)
    {
      api.renderMesh(_meshData);
    }
    virtual void renderDepth(const API& api)
    {}
  };
  template<typename API>
  struct DynamicMeshRenderableWrapper : public MeshRenderable<API>
  {
    DynamicMeshRenderableWrapper(const std::shared_ptr<DynamicMeshRenderable>& dmr,
      const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
      MeshRenderable(material_desc, mesh_data),
      _dmr(dmr)
    {
      fetchShaderDescs();
    }
    std::shared_ptr<DynamicMeshRenderable> _dmr;
    virtual void render(const API& api) override
    {
      const auto& model_matrix = _dmr->getModelMatrix();
      api.renderMesh(_meshData, model_matrix, _dmr->getModelMatrixInverse());
    }
    virtual void renderDepth(const API& api) override
    {
      api.renderMesh(_meshData, _dmr->getModelMatrix());
    }
    virtual AABB* getAABBWorld() const override { return _dmr->getAABBWorld(); }
  };
  template<typename API>
  struct StaticMeshRenderableWrapper : public MeshRenderable<API>
  {
    std::shared_ptr<StaticMeshRenderable> _smr;
    StaticMeshRenderableWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
      MeshRenderable(material_desc, mesh_data),
      _smr(smr)
    {
      fetchShaderDescs();
    }
    virtual ~StaticMeshRenderableWrapper() = default;
    virtual void render(const API& api) override
    {
      api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse());
    }
    virtual void renderDepth(const API& api) override
    {
      api.renderMesh(_meshData, _smr->getModelMatrix());
    }
    virtual AABB* getAABBWorld() const override { return _smr->getAABBWorld(); }
  };
  template<typename API>
  struct StaticMeshRenderableWindWrapper : public StaticMeshRenderableWrapper<API>
  {
    StaticMeshRenderableWindWrapper(const std::shared_ptr<StaticMeshRenderable>& smr,
      const std::shared_ptr<typename API::MaterialDesc>& material_desc, const typename API::MeshGeometryStorage::MeshData& mesh_data) :
      StaticMeshRenderable(smr, material_desc, mesh_data)
    {
      fetchShaderDescs();
    }
    virtual ~StaticMeshRenderableWindWrapper() = default;
    virtual void fetchShaderDescs() override
    {
      _shaderDesc = _materialDesc->getMeshShaderDesc(true).get();
      _shaderDescDepth = _materialDesc->getMeshShaderDescDepth(true).get();
    }
    virtual void render(const API& api) override
    {
      api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getModelMatrixInverse(), _smr->getWindParams(), *getAABBWorld());
    }
    virtual void renderDepth(const API& api) override
    {
      api.renderMesh(_meshData, _smr->getModelMatrix(), _smr->getWindParams(), *getAABBWorld());
    }
  };
}

#endif