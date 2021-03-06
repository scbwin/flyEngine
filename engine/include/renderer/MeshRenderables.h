#ifndef MESHRENDERABLES_H
#define MESHRENDERABLES_H

#include <memory>
#include <functional>
#include <GraphicsSettings.h>
#include <MaterialDesc.h>
#include <Material.h>
#include <AABB.h>
#include <WindParamsLocal.h>
#include <Transform.h>
#include <Sphere.h>
#include <IntersectionTests.h>
#include <Camera.h>
#include <boost/pool/object_pool.hpp>

namespace fly
{
  template<typename API, typename BV>
  class Renderer;
  template<typename API, typename BV>
  class RenderList;

  class LodRenderable
  {
  public:
    LodRenderable() = default;
    virtual ~LodRenderable() = default;
    virtual void selectLod(const Camera::CullingParams& cp) = 0;
  };
  template<typename API>
  class GPURenderable
  {
  public:
    GPURenderable() = default;
    virtual ~GPURenderable() = default;
    virtual void cullGPU(const API& api) = 0;
  };

  template<typename API, typename BV>
  class IMeshRenderable
  {
  public:
    virtual ~IMeshRenderable() = default;
    IMeshRenderable() = default;
    inline std::shared_ptr<ShaderDesc<API>> const * getShaderDesc() const { return _shaderDesc; }
    inline std::shared_ptr<ShaderDesc<API>> const * getShaderDescDepth() const { return _shaderDescDepth; }
    inline MaterialDesc<API> * getMaterialDesc() const { return _materialDesc.get(); }
    virtual void renderDepth(API const & api) const = 0;
    virtual void render(API const & api) const = 0;
    inline const BV& getBV() const { return _bv; }
    /**
    * Called for fully visible meshes.
    */
    virtual void addIfLargeEnough(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist)
    {
      if (largeEnough(cp)) {
        renderlist.addVisibleMesh(this);
      }
    }
    /**
    * Called for potentially visible meshes, i.e. the BVH node intersects the view frustum, but is not fully inside it.
    */
    virtual void addIfLargeEnoughAndVisible(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist)
    {
      if (largeEnough(cp) && intersectFrustum(cp)) {
        renderlist.addVisibleMesh(this);
      }
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
    std::shared_ptr<MaterialDesc<API>> _materialDesc;
    std::shared_ptr<ShaderDesc<API>> const * _shaderDesc;
    std::shared_ptr<ShaderDesc<API>> const * _shaderDescDepth;
    BV _bv;
    void createBV(const Mesh& mesh, const Transform& transform, Sphere& result)
    {
      result = Sphere(mesh, transform);
    }
    void createBV(const Mesh& mesh, const Transform& transform, AABB& result)
    {
      result = AABB(mesh.getAABB(), transform.getModelMatrix());
    }
    inline bool largeEnough(const Camera::CullingParams& cp) const
    {
      return _bv.isLargeEnough(cp._camPos, cp._thresh);
    }
    inline bool intersectFrustum(const Camera::CullingParams& cp) const
    {
      return !IntersectionTests::boundingVolumeOutsideFrustum(_bv, cp._frustumPlanes);
    }
  };
  template<typename API, typename BV>
  class SkydomeRenderable
  {
  public:
    typename API::MeshData _meshData;
    SkydomeRenderable(Renderer<API, BV>& renderer, const std::shared_ptr<Mesh>& mesh) :
      _meshData(renderer.addMesh(mesh))
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
      _materialDesc = renderer.createMaterialDesc(material);
      _shaderDesc = &_materialDesc->getMeshShaderDesc();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepth();
      createBV(*mesh, transform, _bv);
    }
    virtual ~StaticMeshRenderable() = default;
    virtual void render(API const & api) const override
    {
      api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse);
    }
    virtual void renderDepth(API const & api) const override
    {
      api.renderMesh(_meshData, _modelMatrix);
    }
    virtual unsigned numTriangles() const override
    {
      return _meshData.numTriangles();
    }
    void setTransform(const Transform& transform, const std::shared_ptr<Mesh>& mesh)
    {
      _modelMatrix = transform.getModelMatrix();
      _modelMatrixInverse = inverse(glm::mat3(transform.getModelMatrix()));
      createBV(*mesh, transform, _bv);
    }
    const Mat4f& getModelMatrix() const
    {
      return _modelMatrix;
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
      _materialDesc = renderer.createMaterialDesc(material);
      _shaderDesc = &_materialDesc->getMeshShaderDescWind();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepthWind();
      _windParams._pivotWorld = _bv.getMax()[1];
      _windParams._bendFactorExponent = 2.5f;
    }
    virtual void render(API const & api) const override
    {
      api.renderMesh(_meshData, _modelMatrix, _modelMatrixInverse, _windParams, _bv);
    }
    virtual void renderDepth(API const & api) const override
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
  struct InstanceData
  {
    Mat4f _modelMatrix;
    Mat4f _modelMatrixInverse;
    unsigned _index; // Currently an index into a color array
    unsigned _padding[3];
  };
  template<typename API, typename BV>
  class StaticInstancedMeshRenderable : public IMeshRenderable<API, BV>, public GPURenderable<API>
  {
  public:
    StaticInstancedMeshRenderable(Renderer<API, BV>& renderer, const std::vector<std::shared_ptr<Mesh>>& lods,
      const std::shared_ptr<Material>& material, const std::vector<InstanceData>& instance_data) :
      _visibleInstances(renderer.getApi()->createStorageBuffer<unsigned>(nullptr, instance_data.size() * lods.size())),
      _instanceData(renderer.getApi()->createStorageBuffer<InstanceData>(instance_data.data(), instance_data.size())),
      _numInstances(static_cast<unsigned>(instance_data.size()))
    {
      _materialDesc = renderer.createMaterialDesc(material);
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

    virtual void render(API const & api) const override
    {
      api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual void renderDepth(API const & api) const override
    {
      api.renderInstances(_visibleInstances, _indirectBuffer, _instanceData, _indirectInfo, _numInstances);
    }
    virtual float getLargestObjectBVSize() const override
    {
      return _largestBVSize;
    }
    virtual void addIfLargeEnough(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist) override
    {
      if (_bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize)) {
        renderlist.addVisibleMesh(this);
        renderlist.addToGPULodList(this);
      }
    }
    virtual void addIfLargeEnoughAndVisible(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist) override
    {
      if (_bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize)) {
        auto result = IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
        if (result != IntersectionResult::OUTSIDE) {
          renderlist.addVisibleMesh(this);
          result == IntersectionResult::INSIDE ? renderlist.addToGPULodList(this) : renderlist.addToGPUCullList(this);
        }
      }
    }
    virtual unsigned numTriangles() const override
    {
      // TODO calculate correct triangle count, without stalling the pipeline
      return 0;
    }
    virtual unsigned numMeshes() const
    {
      return _numInstances;
    }
    virtual void cullGPU(const API& api) override
    {
      api.cullInstances(_aabbBuffer, _numInstances, _visibleInstances, _indirectBuffer,
        _indirectInfo);
    }
  protected:
    std::vector<typename API::IndirectInfo> _indirectInfo;
    float _largestBVSize = 0.f;
    typename API::StorageBuffer _aabbBuffer;
    typename API::StorageBuffer _visibleInstances;
    typename API::StorageBuffer _instanceData;
    typename API::IndirectBuffer _indirectBuffer;
    unsigned _numInstances;
  };
  template<typename API, typename BV>
  class StaticMeshRenderableLod : public IMeshRenderable<API, BV>, public LodRenderable
  {
  public:
    StaticMeshRenderableLod(Renderer<API, BV>& renderer, const std::vector<std::shared_ptr<Mesh>>& meshes,
      const std::shared_ptr<Material>& material, const Transform& transform) :
      _modelMatrix(transform.getModelMatrix()),
      _modelMatrixInverse(inverse(glm::mat3(transform.getModelMatrix())))
    {
      _materialDesc = renderer.createMaterialDesc(material);
      _shaderDesc = &_materialDesc->getMeshShaderDesc();
      _shaderDescDepth = &_materialDesc->getMeshShaderDescDepth();
      BV bv;
      _meshData.reserve(meshes.size());
      for (const auto& m : meshes) {
        createBV(*m, transform, bv);
        _bv = _bv.getUnion(bv);
        _meshData.push_back(renderer.addMesh(m));
      }
    }
    virtual ~StaticMeshRenderableLod() = default;
    virtual void render(API const & api) const override
    {
      api.renderMesh(_meshData[_lod], _modelMatrix, _modelMatrixInverse);
    }
    virtual void renderDepth(API const & api) const override
    {
      api.renderMesh(_meshData[_lod], _modelMatrix);
    }
    virtual unsigned numTriangles() const override
    {
      return _meshData[_lod].numTriangles();
    }
    virtual void addIfLargeEnough(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist) override
    {
      if (largeEnough(cp)) {
        renderlist.addVisibleMesh(this);
        renderlist.addToCPULodList(this);
      }
    }
    virtual void addIfLargeEnoughAndVisible(const Camera::CullingParams& cp, RenderList<API, BV>& renderlist) override
    {
      if (largeEnough(cp) && intersectFrustum(cp)) {
        renderlist.addVisibleMesh(this);
        renderlist.addToCPULodList(this);
      }
    }
    virtual void selectLod(const Camera::CullingParams& cp) override
    {
      float ratio = _bv.size2() / distance2(_bv.closestPoint(cp._camPos), cp._camPos);
      float alpha = 1.f - std::min((ratio - cp._thresh) / cp._lodRange, 1.f);
      _lod = static_cast<unsigned>(std::roundf(alpha * (_meshData.size() - 1u)));
    }
  protected:
    std::vector<typename API::MeshData> _meshData;
    Mat4f _modelMatrix;
    Mat3f _modelMatrixInverse;
    unsigned _lod;
  };

  template<typename API, typename BV>
  class MeshRenderablePool
  {
    using StaticMeshRenderable = StaticMeshRenderable<API, BV>;
    using StaticMeshRenderableWind = StaticMeshRenderableWind<API, BV>;
    using StaticMeshRenderableLod = StaticMeshRenderableLod<API, BV>;
    using StaticInstancedMeshRenderable = StaticInstancedMeshRenderable<API, BV>;
    using Renderer = Renderer<API, BV>;
  public:
    MeshRenderablePool() = default;
    inline auto* createStaticMeshRenderable(Renderer& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Transform& transform)
    {
      return new(_poolSmr.malloc()) StaticMeshRenderable(renderer, mesh, material, transform);
    }
    inline auto* createStaticMeshRenderableWind(Renderer& renderer, const std::shared_ptr<Mesh>& mesh,
      const std::shared_ptr<Material>& material, const Transform& transform)
    {
      return new(_poolSmrWind.malloc()) StaticMeshRenderableWind(renderer, mesh, material, transform);
    }
    inline auto* createStaticMeshRenderableLod(Renderer& renderer, const std::vector<std::shared_ptr<Mesh>>& meshes,
      const std::shared_ptr<Material>& material, const Transform& transform)
    {
      return new(_poolSmrLod.malloc()) StaticMeshRenderableLod(renderer, meshes, material, transform);
    }
    inline auto* createStaticInstancedMeshRenderable(Renderer& renderer, const std::vector<std::shared_ptr<Mesh>>& lods,
      const std::shared_ptr<Material>& material, const std::vector<InstanceData>& instance_data)
    {
      return new(_poolSmrInstanced.malloc()) StaticInstancedMeshRenderable(renderer, lods, material, instance_data);
    }
  private:
    boost::object_pool<StaticMeshRenderable> _poolSmr;
    boost::object_pool<StaticMeshRenderableWind> _poolSmrWind;
    boost::object_pool<StaticMeshRenderableLod> _poolSmrLod;
    boost::object_pool<StaticInstancedMeshRenderable> _poolSmrInstanced;
  };
}

#endif // !MESHRENDERABLES_H
