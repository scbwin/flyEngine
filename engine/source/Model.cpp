#define GLM_ENABLE_EXPERIMENTAL
#include <Model.h>
#include <Mesh.h>

namespace fly
{
  Model::Model(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::vector<std::shared_ptr<Material>>& materials) : _meshes(meshes), _materials(materials)
  {
    sortMeshesByMaterial();
    Vec3f bb_min(std::numeric_limits<float>::max());
    Vec3f bb_max(std::numeric_limits<float>::lowest());
    for (const auto& m : _meshes) {
      bb_min = minimum(bb_min, m->getAABB()->getMin());
      bb_max = maximum(bb_max, m->getAABB()->getMax());
    }
    _aabb = std::make_unique<AABB>(bb_min, bb_max);
  }

  Model::Model(const Model& other) : 
    _meshes(other._meshes), 
    _materials(other._materials), 
    _aabb(std::make_unique<AABB>(*other.getAABB()))
  {
  }

  const std::vector<std::shared_ptr<Mesh>>& Model::getMeshes() const
  {
    return _meshes;
  }

  const std::vector<std::shared_ptr<Material>>& Model::getMaterials() const
  {
    return _materials;
  }

  std::vector<std::shared_ptr<Material>> Model::copyMaterials() const
  {
    std::vector<std::shared_ptr<Material>> ret;
    for (const auto& m : _materials) {
      ret.push_back(std::make_shared<Material>(*m));
    }
    return ret;
  }

  void Model::setMeshes(const std::vector<std::shared_ptr<Mesh>>& meshes)
  {
    _meshes = meshes;
  }

  void Model::setMaterials(const std::vector<std::shared_ptr<Material>>& materials)
  {
    _materials = materials;
  }

  void Model::sortMeshesByMaterial()
  {
    std::sort(_meshes.begin(), _meshes.end(), [](const std::shared_ptr<Mesh>& m1, const std::shared_ptr<Mesh>& m2) {
      return m1->getMaterialIndex() > m2->getMaterialIndex();
    });
  }
  AABB * Model::getAABB() const
  {
    return _aabb.get();
  }
}