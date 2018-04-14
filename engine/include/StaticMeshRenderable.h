#ifndef STATICMESHRENDERABLE_H
#define STATICMESHRENDERABLE_H

#include <Component.h>
#include <vector>
#include <memory>
#include <math/FlyMath.h>
#include <WindParamsLocal.h>

namespace fly
{
  class Transform;
  class AABB;
  class Mesh;
  class Material;

  class StaticMeshRenderable : public Component
  {
  public:
    StaticMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, 
      const Mat4f& model_matrix, bool has_wind, const Vec3f& aabb_offset = Vec3f(0.f));
    AABB* getAABBWorld() const;
    const std::shared_ptr<Mesh>& getMesh() const;
    const std::shared_ptr<Material>& getMaterial() const;
    const Mat4f& getModelMatrix() const;
    const Mat3f& getModelMatrixInverse() const;
    bool hasWind() const;
    void setHasWind(bool has_wind);
    const WindParamsLocal& getWindParams() const;
    void setWindParams(const WindParamsLocal& params);
  private:
    Mat4f _modelMatrix;
    Mat3f _modelMatrixInverse;
    std::shared_ptr<Mesh> _mesh;
    std::shared_ptr<Material> _material;
    std::unique_ptr<AABB> _aabbWorld;
    bool _hasWind;
    WindParamsLocal _windParams;
  };
}

#endif
