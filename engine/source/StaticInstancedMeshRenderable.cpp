#include <StaticInstancedMeshRenderable.h>

namespace fly
{
  StaticInstancedMeshRenderable::StaticInstancedMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const AABB& aabb_local, const std::vector<Mat4f>& model_matrices) :
    _mesh(mesh),
    _material(material),
    _modelMatrices(model_matrices)
  {
    for (const auto& m : model_matrices) {
      _aabbsWorld.push_back(AABB(aabb_local, m));
      _modelMatricesInverse.push_back(transpose(inverse(glm::mat4(m))));
    }
    for (const auto& aabb : _aabbsWorld) {
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
  const std::shared_ptr<Mesh>& StaticInstancedMeshRenderable::getMesh() const
  {
    return _mesh;
  }
  const std::shared_ptr<Material>& StaticInstancedMeshRenderable::getMaterial() const
  {
    return _material;
  }
  AABB const * StaticInstancedMeshRenderable::getAABBWorld() const
  {
    return &_aabb;
  }
}