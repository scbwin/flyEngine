#include <StaticInstancedMeshRenderable.h>
#include <Mesh.h>

namespace fly
{
  StaticInstancedMeshRenderable::StaticInstancedMeshRenderable(const std::vector<std::shared_ptr<Mesh>>& meshes, 
    const std::shared_ptr<Material>& material, const std::vector<Mat4f>& model_matrices, float lod_multiplier) :
    _meshes(meshes),
    _material(material),
    _modelMatrices(model_matrices),
    _lodMultiplier(lod_multiplier)
  {
    AABB aabb_local;
    for (const auto& m : _meshes) { // The bounding box that encloses all LODs is computed.
      aabb_local = aabb_local.getUnion(*m->getAABB());
    }
    for (const auto& m : model_matrices) {
      _aabbsWorld.push_back(AABB(aabb_local, m));
      _largestAABBSize = std::max(_largestAABBSize, _aabbsWorld.back().size2());
      _modelMatricesInverse.push_back(transpose(inverse(glm::mat4(m))));
    }
    for (const auto& aabb : _aabbsWorld) { // Bounding box that encloses all instances
      _aabb = _aabb.getUnion(aabb);
    }
  }
  const std::vector<AABB>& StaticInstancedMeshRenderable::getAABBsWorld() const
  {
    return _aabbsWorld;
  }
  const std::vector<Mat4f>& StaticInstancedMeshRenderable::getModelMatrices() const
  {
    return _modelMatrices;
  }
  const std::vector<Mat4f>& StaticInstancedMeshRenderable::getModelMatricesInverse() const
  {
    return _modelMatricesInverse;
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
    _modelMatrices = std::vector<Mat4f>();
    _modelMatricesInverse = std::vector<Mat4f>();
  }
}