#ifndef BULLET3PHYSICSSYSTEM_H
#define BULLET3PHYSICSSYSTEM_H

#include <System.h>
#include <memory>
#include <map>

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;

namespace fly
{
  class RigidBody;
  class Bullet3PhysicsSystem : public System
  {
  public:
    Bullet3PhysicsSystem();
    virtual ~Bullet3PhysicsSystem() = default;
    void setSimulationSubsteps(int steps);
    int getSimulationSubsteps() const;
  private:
    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override;
    virtual void update(float time, float delta_time) override;

    std::unique_ptr<btBroadphaseInterface> _iBroadphase;
    std::unique_ptr<btDefaultCollisionConfiguration> _collisionConfig;
    std::unique_ptr<btCollisionDispatcher> _collisionDispatcher;
    std::unique_ptr<btSequentialImpulseConstraintSolver> _solver;
    std::unique_ptr<btDiscreteDynamicsWorld> _world;

    int _simulationSubSteps = 5;

    std::map<Entity*, std::shared_ptr<RigidBody>> _rigidBodys;
  };
}

#endif
