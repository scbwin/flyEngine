#ifndef MESHRENDERABLES_H
#define MESHRENDERABLES_H

#include <memory>
#include <functional>
#include <GraphicsSettings.h>
#include <MaterialDesc.h>
#include <Material.h>
#include <AABB.h>
#include <Component.h>
#include <WindParamsLocal.h>

namespace fly
{
  template<typename API>
  class Renderer;

  template<typename API>
  class IMeshRenderable : public Component
  {
  public:
    virtual ~IMeshRenderable() = default;
    IMeshRenderable() = default;
    inline std::shared_ptr<ShaderDesc<API>> const * getShaderDesc() { return _shaderDesc; }
    inline std::shared_ptr<ShaderDesc<API>> const * getShaderDescDepth() { return _shaderDescDepth; }
    inline MaterialDesc<API> * getMaterialDesc() { return _materialDesc; }
    virtual void renderDepth(API const & api) = 0;
    virtual void render(API const & api) = 0;
    inline const AABB& getAABB() { return _aabb; }
    virtual bool isLargeEnough(const Camera& camera) const
    {
      return _aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold());
    }
    virtual bool cull(const Camera& c) // Fine-grained distance culling, called if the object is fully visible from the camera's point of view.
    {
      return isLargeEnough(c);
    }
    virtual bool cullAndIntersect(const Camera& c) // Fine-grained culling, only called if the bounding box of the node the object is in intersects the view frustum.
    {
      return cull(c) && c.frustumIntersectsAABB(_aabb) != IntersectionResult::OUTSIDE;
    }
    virtual float getLargestObjectAABBSize() const
    {
      return _aabb.size2();
    }
    virtual unsigned numMeshes() const
    {
      return 1;
    }
    virtual unsigned numTriangles() const = 0;
  protected:
    MaterialDesc<API> * _materialDesc;
    std::shared_ptr<ShaderDesc<API>> const * _shaderDesc;
    std::shared_ptr<ShaderDesc<API>> const * _shaderDescDepth;
    AABB _aabb;
  };
  template<typename API>
  class SkydomeRenderableWrapper
  {
  public:
    API const & _api;
    typename API::MeshData _meshData;
    SkydomeRenderableWrapper(const typename API::MeshData& mesh_data, API const & api) :
      _api(api),
      _meshData(mesh_data)
    {
    }
    void render()
    {
      _api.renderMesh(_meshData);
    }
  };
  template<typename API>
  class StaticMeshRenderable : public IMeshRenderable<API>
  {
  public:
    StaticMeshRenderable(Renderer<API>& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Mat4f& model_matrix) :
      _meshData(renderer.addMesh(mesh)),
      _modelMatrix(model_matrix),
      _modelMatrixInverse(inverse(glm::mat3(model_matrix)))
    {
      _materialDesc = renderer.createMaterialDesc(material).get();
      _shaderDesc = &_materialDesc->getMeshShaderDesc();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepth();
      _aabb = AABB(mesh->getAABB(), model_matrix);
    }
    virtual ~StaticMeshRenderable() = default;
    virtual void render(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse);
    }
    virtual void renderDepth(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix);
    }
    virtual unsigned numTriangles() const override
    {
      return _meshData.numTriangles();
    }
  protected:
    typename API::MeshData _meshData;
    Mat4f _modelMatrix;
    Mat3f _modelMatrixInverse;
  };
  template<typename API>
  class StaticMeshRenderableWind : public StaticMeshRenderable<API>
  {
  public:
    StaticMeshRenderableWind(Renderer<API>& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Mat4f& model_matrix) :
      StaticMeshRenderable<API>(renderer, mesh, material, model_matrix)
    {
      _materialDesc = renderer.createMaterialDesc(material).get();
      _shaderDesc = &_materialDesc->getMeshShaderDescWind();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepthWind();
      _windParams._pivotWorld = _aabb.getMax()[1];
      _windParams._bendFactorExponent = 2.5f;
    }
    virtual void render(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse, _windParams, _aabb);
    }
    virtual void renderDepth(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix, _windParams, _aabb);
    }
    void setWindParams(const WindParamsLocal& params)
    {
      _windParams = params;
    }
    void expandAABB(const Vec3f& amount)
    {
      _aabb.expand(amount);
    }
  protected:
    WindParamsLocal _windParams;
  };
  template<typename API>
  class StaticInstancedMeshRenderable : public IMeshRenderable<API>
  {
  public:
    struct InstanceData
    {
      Mat4f _modelMatrix;
      Mat4f _modelMatrixInverse;
      unsigned _index; // Can be an index into a color array or an index into a texture array
      unsigned _padding[3];
    };
    StaticInstancedMeshRenderable(Renderer<API>& renderer, const std::vector<std::shared_ptr<Mesh>>& lods,
      const std::shared_ptr<Material>& material, const std::vector<InstanceData>& instance_data) :
      _visibleInstances(renderer.getApi()->createStorageBuffer<unsigned>(nullptr, instance_data.size() * lods.size())),
      _instanceData(renderer.getApi()->createStorageBuffer<InstanceData>(instance_data.data(), instance_data.size())),
      _numInstances(static_cast<unsigned>(instance_data.size())),
      _api(*renderer.getApi())
    {
      _materialDesc = renderer.createMaterialDesc(material).get();
      _shaderDesc = &_materialDesc->getMeshShaderDescInstanced();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepthInstanced();
      std::vector<typename API::MeshData> mesh_data;
      for (const auto& m : lods) {
        mesh_data.push_back(renderer.addMesh(m));
      }
      _indirectInfo = renderer.getApi()->indirectFromMeshData(mesh_data);
      _indirectBuffer = renderer.getApi()->createIndirectBuffer(_indirectInfo);

      AABB aabb_local;
      for (const auto& m : lods) {
        aabb_local = aabb_local.getUnion(m->getAABB());
      }

      StackPOD<Vec4f> aabbs_min_max;
      aabbs_min_max.reserve(instance_data.size() * 2u);
      for (unsigned i = 0; i < instance_data.size(); i++) {
        AABB aabb_world(aabb_local, instance_data[i]._modelMatrix);
        aabbs_min_max.push_back(Vec4f(aabb_world.getMin(), 1.f));
        aabbs_min_max.push_back(Vec4f(aabb_world.getMax(), 1.f));
        _aabb = _aabb.getUnion(aabb_world);
        _largestAABBSize = std::max(_largestAABBSize, aabb_world.size2());
      }
      _aabbBuffer = std::move(renderer.getApi()->createStorageBuffer<Vec4f>(aabbs_min_max.begin(), aabbs_min_max.size()));
    }
    virtual ~StaticInstancedMeshRenderable() = default;

    virtual void render(API const & api) override
    {
      api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual void renderDepth(API const & api) override
    {
      api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual float getLargestObjectAABBSize() const override
    {
      return _largestAABBSize;
    }
    virtual bool cull(const Camera& camera) override
    {
      if (isLargeEnough(camera)) {
        _api.cullInstances(_aabbBuffer, _numInstances, _visibleInstances, _indirectBuffer,
          _indirectInfo, _lodMultiplier, camera.getDetailCullingThreshold());
        return true;
      }
      return false;
    }
    virtual bool cullAndIntersect(const Camera& camera) override
    {
      return camera.frustumIntersectsAABB(_aabb) != IntersectionResult::OUTSIDE && cull(camera);
    }
    virtual bool isLargeEnough(const Camera& camera) const
    {
      return _aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize);
    }
    virtual unsigned numTriangles() const override
    {
      // TODO calculate correct triangle count here
      return 0;
    }
    virtual unsigned numMeshes() const
    {
      return _numInstances;
    }
  protected:
    std::vector<typename API::IndirectInfo> _indirectInfo;
    float _largestAABBSize = 0.f;
    typename API::StorageBuffer _aabbBuffer;
    typename API::StorageBuffer _visibleInstances;
    typename API::StorageBuffer _instanceData;
    typename API::IndirectBuffer _indirectBuffer;
    unsigned _numInstances;
    API const & _api;
    float _lodMultiplier = 0.1f;
  };
}

#endif // !MESHRENDERABLES_H
