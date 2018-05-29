#define GLM_ENABLE_EXPERIMENTAL
#include <Model.h>
#include <Mesh.h>
#include <map>

namespace fly
{
  Model::Model(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::vector<std::shared_ptr<Material>>& materials) : 
    _meshes(meshes), 
    _materials(materials),
    _aabb(*this)
  {
    sortMeshesByMaterial();
  }

  Model::Model(const Model& other) : 
    _meshes(other._meshes), 
    _materials(other._materials), 
    _aabb(other._aabb)
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
  void Model::mergeMeshesByMaterial()
  {
    std::map<unsigned, std::vector<std::shared_ptr<Mesh>>> _meshesGrouped;
    for (const auto& m : _meshes) {
      _meshesGrouped[m->getMaterialIndex()].push_back(m);
    }
    _meshes.clear();
    for (const auto& e : _meshesGrouped) {
      unsigned base_vertex = 0;
      std::vector<Vertex> vertices;
      std::vector<unsigned> indices;
      for (const auto& m : e.second) {
        vertices.insert(vertices.end(), m->getVertices().begin(), m->getVertices().end());
        for (const auto& i : m->getIndices()) {
          indices.push_back(i + base_vertex);
        }
        base_vertex += m->getVertices().size();
      }
      _meshes.push_back(std::make_shared<Mesh>(vertices, indices, e.first));
      _meshes.back()->setMaterial(_materials[e.first]);
    }
  }
  const AABB& Model::getAABB() const
  {
    return _aabb;
  }
}