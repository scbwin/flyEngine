#include <DynamicMeshRenderable.h>
#include <physics/RigidBody.h>
#include <btBulletDynamicsCommon.h>
#include <AABB.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>

namespace fly
{
  DynamicMeshRenderable::DynamicMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const std::shared_ptr<RigidBody>& rigid_body) :
    _mesh(mesh),
    _material(material),
    _rigidBody(rigid_body),
    _aabb(Vec3f(0.f), Vec3f(0.f))
  {
  }
  DynamicMeshRenderable::~DynamicMeshRenderable()
  {
  }
  const std::shared_ptr<Mesh>& DynamicMeshRenderable::getMesh() const
  {
    return _mesh;
  }
  const std::shared_ptr<Material>& DynamicMeshRenderable::getMaterial() const
  {
    return _material;
  }
  const Mat4f& DynamicMeshRenderable::getModelMatrix()
  {
    _rigidBody->getBtRigidBody()->getWorldTransform().getOpenGLMatrix(&_modelMatrix[0][0]);
    const auto& scaling = _rigidBody->getBtRigidBody()->getCollisionShape()->getLocalScaling();
    Vec3f s(scaling.x(), scaling.y(), scaling.z());
    _modelMatrix = _modelMatrix * scale<4, float>(s);
    return _modelMatrix;
  }
  const Mat3f& DynamicMeshRenderable::getModelMatrixInverse()
  {
    _modelMatrixInverse = inverse(glm::mat3(_modelMatrix));
    return _modelMatrixInverse;
  }
  AABB* DynamicMeshRenderable::getAABBWorld()
  {
    btVector3 min, max;
    _rigidBody->getBtRigidBody()->getAabb(min, max);
    _aabb = AABB(Vec3f(min.x(), min.y(), min.z()), Vec3f(max.x(), max.y(), max.z()));
    return &_aabb;
  }
}