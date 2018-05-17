#ifndef MODEL_H
#define MODEL_H

#include <Component.h>
#include <memory>
#include <vector>
#include <Material.h>
#include <AABB.h>

namespace fly
{
  class Mesh;

  class Model : public Component
  {
  public:
    Model(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::vector<std::shared_ptr<Material>>& materials);
    Model(const Model& other);
    const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
    const std::vector<std::shared_ptr<Material>>& getMaterials() const;
    std::vector<std::shared_ptr<Material>> copyMaterials() const;
    void setMeshes(const std::vector<std::shared_ptr<Mesh>>& meshes);
    void setMaterials(const std::vector<std::shared_ptr<Material>>& materials);
    void sortMeshesByMaterial();
    void mergeMeshesByMaterial();
    AABB const * getAABB() const;

  private:
    std::vector<std::shared_ptr<Mesh>> _meshes;
    std::vector<std::shared_ptr<Material>> _materials;
    AABB _aabb;
  };
}

#endif