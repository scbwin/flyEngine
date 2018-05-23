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
    StaticInstancedMeshRenderable(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::shared_ptr<Material>& material, 
      const std::vector<Mat4f>& model_matrices, const std::vector<unsigned>& indices, float lod_multiplier);
    const std::vector<AABB>& getAABBsWorld() const;
    const std::vector<std::shared_ptr<Mesh>>& getMeshes() const;
    const std::shared_ptr<Material>& getMaterial() const;
    AABB const * getAABBWorld() const;
    float getLodMultiplier() const;
    void setLodMultiplier(float lod_multiplier);
    float getLargestAABBSize() const;
    void clear();
    struct InstanceData
    {
      Mat4f _modelMatrix;
      Mat4f _modelMatrixInverse;
      unsigned _index; // Can be an index into a color array or an index into a texture array
      unsigned _padding[3];
    };
    const std::vector<InstanceData> getInstanceData() const;
  private:
    std::vector<AABB> _aabbsWorld;
    std::vector<InstanceData> _instanceData;
    std::vector<std::shared_ptr<Mesh>> _meshes;
    std::shared_ptr<Material> _material;
    AABB _aabb;
    float _lodMultiplier;
    float _largestAABBSize = 0.f;
  };
}

#endif // !STATICINSTANCEDMESHRENDERABLE_H
