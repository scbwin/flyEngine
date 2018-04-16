#ifndef DYNAMICMESHRENDERABLE_H
#define DYNAMICMESHRENDERABLE_H

#include <memory>
#include <math/FlyMath.h>
#include <Component.h>
#include <AABB.h>

namespace fly
{
  class RigidBody;
  class Transform;
  class Mesh;
  class Material;


  class DynamicMeshRenderable : public Component
  {
  public:
    DynamicMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material,  const std::shared_ptr<RigidBody>& rigid_body);
    ~DynamicMeshRenderable();
    const std::shared_ptr<Mesh>& getMesh() const;
    const std::shared_ptr<Material>& getMaterial() const;
    const Mat4f& getModelMatrix();
    const Mat3f& getModelMatrixInverse();
    AABB* getAABBWorld();
  private:
    std::shared_ptr<RigidBody> _rigidBody;
    std::shared_ptr<Mesh> _mesh;
    std::shared_ptr<Material> _material;
    Mat4f _modelMatrix;
    Mat3f _modelMatrixInverse;
    AABB _aabb;
  };
}

#endif
