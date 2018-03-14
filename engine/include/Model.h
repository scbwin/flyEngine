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
    Model(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::vector<Material>& materials);
    Model(const Model& other);
    std::vector<std::shared_ptr<Mesh>>& getMeshes();
    std::vector<Material>& getMaterials();
    void sortMeshesByMaterial();
    AABB* getAABB() const;

  private:
    std::vector<std::shared_ptr<Mesh>> _meshes;
    std::vector<Material> _materials;
    std::unique_ptr<AABB> _aabb;
  };
}

#endif