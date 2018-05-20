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
    StaticInstancedMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const AABB& aabb_local, const std::vector<Mat4f>& model_matrices);
    const std::vector<AABB>& getAABBsWorld() const;
    const std::vector<Mat4f>& getModelMatrices() const;
    const std::vector<Mat4f>& getModelMatricesInverse() const;
    const std::shared_ptr<Mesh>& getMesh() const;
    const std::shared_ptr<Material>& getMaterial() const;
    AABB const * getAABBWorld() const;
  private:
    std::vector<AABB> _aabbsWorld;
    std::vector<Mat4f> _modelMatrices;
    std::vector<Mat4f> _modelMatricesInverse;
    std::shared_ptr<Mesh> _mesh;
    std::shared_ptr<Material> _material;
    AABB _aabb;
  };
}

#endif // !STATICINSTANCEDMESHRENDERABLE_H
