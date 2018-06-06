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
#include <Transform.h>
#include <Sphere.h>

namespace fly
{
  template<typename API, typename BV = AABB>
  class Renderer;

  template<typename API, typename BV>
  class IMeshRenderable : public Component
  {
  public:
    virtual ~IMeshRenderable() = default;
    IMeshRenderable() = default;
    inline std::shared_ptr<ShaderDesc<API, BV>> const * getShaderDesc() { return _shaderDesc; }
    inline std::shared_ptr<ShaderDesc<API, BV>> const * getShaderDescDepth() { return _shaderDescDepth; }
    inline MaterialDesc<API, BV> * getMaterialDesc() { return _materialDesc; }
    virtual void renderDepth(API const & api) = 0;
    virtual void render(API const & api) = 0;
    inline const BV& getBV() const { return _bv; }
    virtual bool isLargeEnough(const Camera& camera) const
    {
      return _bv.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold());
    }
    virtual bool cull(const Camera& c) // Fine-grained distance culling, called if the object is fully visible from the camera's point of view.
    {
      return isLargeEnough(c);
    }
    virtual bool cullAndIntersect(const Camera& c) // Fine-grained culling, only called if the bounding box of the node the object is in intersects the view frustum.
    {
      return cull(c) && c.frustumIntersectsBoundingVolume(_bv) != IntersectionResult::OUTSIDE;
    }
    virtual float getLargestObjectBVSize() const
    {
      return _bv.size2();
    }
    virtual unsigned numMeshes() const
    {
      return 1;
    }
    virtual unsigned numTriangles() const = 0;
  protected:
    MaterialDesc<API, BV> * _materialDesc;
    std::shared_ptr<ShaderDesc<API, BV>> const * _shaderDesc;
    std::shared_ptr<ShaderDesc<API, BV>> const * _shaderDescDepth;
    BV _bv;
    void createBV(const Mesh& mesh, const Transform& transform, Sphere& result)
    {
      result = Sphere(mesh, transform);
    }
    void createBV(const Mesh& mesh, const Transform& transform, AABB& result)
    {
      result = AABB(mesh.getAABB(), transform.getModelMatrix());
    }
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
    const typename API::MeshData& getMeshData() const
    {
      return _meshData;
    }
  };
  template<typename API, typename BV>
  class StaticMeshRenderable : public IMeshRenderable<API, BV>
  {
  public:
    StaticMeshRenderable(Renderer<API, BV>& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Transform& transform) :
      _meshData(renderer.addMesh(mesh)),
      _modelMatrix(transform.getModelMatrix()),
      _modelMatrixInverse(inverse(glm::mat3(transform.getModelMatrix())))
    {
      _materialDesc = renderer.createMaterialDesc(material).get();
      _shaderDesc = &_materialDesc->getMeshShaderDesc();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepth();
      createBV(*mesh, transform, _bv);
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
  template<typename API, typename BV>
  class StaticMeshRenderableWind : public StaticMeshRenderable<API, BV>
  {
  public:
    StaticMeshRenderableWind(Renderer<API, BV>& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Transform& transform) :
      StaticMeshRenderable<API, BV>(renderer, mesh, material, transform)
    {
      _materialDesc = renderer.createMaterialDesc(material).get();
      _shaderDesc = &_materialDesc->getMeshShaderDescWind();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepthWind();
      _windParams._pivotWorld = _bv.getMax()[1];
      _windParams._bendFactorExponent = 2.5f;
    }
    virtual void render(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse, _windParams, _bv);
    }
    virtual void renderDepth(API const & api) override
    {
      api.renderMesh(_meshData, _modelMatrix, _windParams, _bv);
    }
    void setWindParams(const WindParamsLocal& params)
    {
      _windParams = params;
    }
    void expandAABB(const Vec3f& amount)
    {
      _bv.expand(amount);
    }
  protected:
    WindParamsLocal _windParams;
  };
  template<typename API, typename BV>
  class StaticInstancedMeshRenderable : public IMeshRenderable<API, BV>
  {
  public:
    struct InstanceData
    {
      Mat4f _modelMatrix;
      Mat4f _modelMatrixInverse;
      unsigned _index; // Can be an index into a color array or an index into a texture array
      unsigned _padding[3];
    };
    StaticInstancedMeshRenderable(Renderer<API, BV>& renderer, const std::vector<std::shared_ptr<Mesh>>& lods,
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

      StackPOD<Vec4f> bbs_min_max;
      bbs_min_max.reserve(instance_data.size() * 2u);
      for (unsigned i = 0; i < instance_data.size(); i++) {
        AABB aabb_world(aabb_local, instance_data[i]._modelMatrix);
        bbs_min_max.push_back(Vec4f(aabb_world.getMin(), 1.f));
        bbs_min_max.push_back(Vec4f(aabb_world.getMax(), 1.f));
        _bv = _bv.getUnion(aabb_world);
        _largestBVSize = std::max(_largestBVSize, aabb_world.size2());
      }
      _aabbBuffer = std::move(renderer.getApi()->createStorageBuffer<Vec4f>(bbs_min_max.begin(), bbs_min_max.size()));
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
    virtual float getLargestObjectBVSize() const override
    {
      return _largestBVSize;
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
      return camera.frustumIntersectsBoundingVolume(_bv) != IntersectionResult::OUTSIDE && cull(camera);
    }
    virtual bool isLargeEnough(const Camera& camera) const
    {
      return _bv.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestBVSize);
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
    float _largestBVSize = 0.f;
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
