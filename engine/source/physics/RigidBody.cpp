#include <physics/RigidBody.h>
#include <btBulletDynamicsCommon.h>

namespace fly
{
  RigidBody::RigidBody(const Vec3f & position, float mass, const std::shared_ptr<btCollisionShape>& col_shape, float restitution, float linear_damping, float angular_damping) :
    _collisionShape(col_shape)
  {
    _motionState = std::make_unique<btDefaultMotionState>(btTransform(btQuaternion(0, 0, 0, 1), btVector3(position[0], position[1], position[2])));
    btVector3 inertia;
    if (mass > 0.f) col_shape->calculateLocalInertia(mass, inertia);
    btRigidBody::btRigidBodyConstructionInfo info(mass, _motionState.get(), col_shape.get(), inertia);
    _rigidBody = std::make_unique<btRigidBody>(info);
    _rigidBody->setRestitution(restitution);
    _rigidBody->setDamping(linear_damping, angular_damping);
  }
  RigidBody::~RigidBody()
  {
  }
  const std::unique_ptr<btRigidBody>& RigidBody::getBtRigidBody() const
  {
    return _rigidBody;
  }
}