#ifndef STATICINSTANCEDMESHRENDERABLE_H
#define STATICINSTANCEDMESHRENDERABLE_H

#include <Component.h>
#include <memory>
#include <vector>
#include <AABB.h>
#include <math/FlyMath.h>

namespace fly
{
  class AABB;
  class Mesh;
  class Material;

  class StaticInstancedMeshRenderable : public Component
  {
  public:
    StaticInstancedMeshRenderable(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::shared_ptr<Material>& material, const std::vector<Mat4f>& model_matrices, float lod_multiplier);
    const std::vector<AABB>& getAABBsWorld() const;
    const std::vector<Mat4f>& getModelMatrices() const;
    const std::vector<Mat4f>& getModelMatricesInverse() const;
    const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
    const std::shared_ptr<Material>& getMaterial() const;
    AABB const * getAABBWorld() const;
    float getLodMultiplier() const;
    void setLodMultiplier(float lod_multiplier);
  private:
    std::vector<AABB> _aabbsWorld;
    std::vector<Mat4f> _modelMatrices;
    std::vector<Mat4f> _modelMatricesInverse;
    std::vector<std::shared_ptr<Mesh>> _meshes;
    std::shared_ptr<Material> _material;
    AABB _aabb;
    float _lodMultiplier;
  };
}

#endif // !STATICINSTANCEDMESHRENDERABLE_H
