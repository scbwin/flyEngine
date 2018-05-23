#include <StaticInstancedMeshRenderable.h>
#include <Mesh.h>

namespace fly
{
  StaticInstancedMeshRenderable::StaticInstancedMeshRenderable(const std::vector<std::shared_ptr<Mesh>>& meshes, 
    const std::shared_ptr<Material>& material, const std::vector<Mat4f>& model_matrices, const std::vector<unsigned>& indices, float lod_multiplier) :
    _meshes(meshes),
    _material(material),
    _lodMultiplier(lod_multiplier)
  {
    AABB aabb_local;
    for (const auto& m : _meshes) { // The bounding box that encloses all LODs is computed.
      aabb_local = aabb_local.getUnion(*m->getAABB());
    }
    for (unsigned i = 0; i < model_matrices.size(); i++) {
      _aabbsWorld.push_back(AABB(aabb_local, model_matrices[i]));
      _largestAABBSize = std::max(_largestAABBSize, _aabbsWorld.back().size2());
      _instanceData.push_back({ model_matrices[i], transpose(inverse(glm::mat4(model_matrices[i]))), indices[i] });
    }
    for (const auto& aabb : _aabbsWorld) { // Bounding box that encloses all instances
      _aabb = _aabb.getUnion(aabb);
    }
  }
  const std::vector<AABB>& StaticInstancedMeshRenderable::getAABBsWorld() const
  {
    return _aabbsWorld;
  }
  const std::vector<std::shared_ptr<Mesh>>& StaticInstancedMeshRenderable::getMeshes() const
  {
    return _meshes;
  }
  const std::shared_ptr<Material>& StaticInstancedMeshRenderable::getMaterial() const
  {
    return _material;
  }
  AABB const * StaticInstancedMeshRenderable::getAABBWorld() const
  {
    return &_aabb;
  }
  float StaticInstancedMeshRenderable::getLodMultiplier() const
  {
    return _lodMultiplier;
  }
  void StaticInstancedMeshRenderable::setLodMultiplier(float lod_multiplier)
  {
    _lodMultiplier = lod_multiplier;
  }
  float StaticInstancedMeshRenderable::getLargestAABBSize() const
  {
    return _largestAABBSize;
  }
  void StaticInstancedMeshRenderable::clear()
  {
    _aabbsWorld = std::vector<AABB>();
    _instanceData = std::vector<InstanceData>();
  }
  const std::vector<StaticInstancedMeshRenderable::InstanceData> StaticInstancedMeshRenderable::getInstanceData() const
  {
    return _instanceData;
  }
}