#include <physics/Bullet3PhysicsSystem.h>
#include <btBulletDynamicsCommon.h>
#include <Entity.h>
#include <physics/RigidBody.h>

namespace fly
{
  Bullet3PhysicsSystem::Bullet3PhysicsSystem() :
    _iBroadphase(std::make_unique<btDbvtBroadphase>()),
    _collisionConfig(std::make_unique<btDefaultCollisionConfiguration>()),
    _collisionDispatcher(std::make_unique<btCollisionDispatcher>(_collisionConfig.get())),
    _solver(std::make_unique<btSequentialImpulseConstraintSolver>()),
    _world(std::make_unique<btDiscreteDynamicsWorld>(_collisionDispatcher.get(), _iBroadphase.get(), _solver.get(), _collisionConfig.get()))
  {
    _world->setGravity(btVector3(0.f, -10.f, 0.f));
  }
  void Bullet3PhysicsSystem::setSimulationSubsteps(int steps)
  {
    _simulationSubSteps = steps;
  }
  int Bullet3PhysicsSystem::getSimulationSubsteps() const
  {
    return _simulationSubSteps;
  }
  void Bullet3PhysicsSystem::onComponentAdded(Entity * entity, const std::shared_ptr<Component>& component)
  {
    if (auto c = addIfInterested<RigidBody>(entity, component, _rigidBodys)) {
      _world->addRigidBody(c->getBtRigidBody().get());
    }
  }
  void Bullet3PhysicsSystem::onComponentRemoved(Entity * entity, const std::shared_ptr<Component>& component)
  {
    if (auto c = deleteIfInterested<RigidBody>(entity, component, _rigidBodys)) {
      _world->removeRigidBody(c->getBtRigidBody().get());
    }
  }
  void Bullet3PhysicsSystem::update(float time, float delta_time)
  {
    _world->stepSimulation(delta_time, _simulationSubSteps);
  }
}