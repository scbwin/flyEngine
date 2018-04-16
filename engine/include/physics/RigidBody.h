#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <memory>
#include <Component.h>
#include <math/FlyMath.h>

class btCollisionShape;
class btRigidBody;
struct btDefaultMotionState;

namespace fly
{
  class RigidBody : public Component
  {
  public:
    RigidBody(const Vec3f& position, float mass, const std::shared_ptr<btCollisionShape>& col_shape, float resitution = 1.f, float linear_damping = 0.05f, float angular_damping = 0.05f);
    ~RigidBody();
    const std::unique_ptr<btRigidBody>& getBtRigidBody() const;
  private:
    std::unique_ptr<btRigidBody> _rigidBody;
    std::unique_ptr<btDefaultMotionState> _motionState;
    std::shared_ptr<btCollisionShape> _collisionShape;
  };
}

#endif
