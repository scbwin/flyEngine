#include <physics/Bullet3PhysicsSystem.h>
#include <btBulletDynamicsCommon.h>
#include <physics/RigidBody.h>
#include <GameTimer.h>

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
  const std::unique_ptr<btDiscreteDynamicsWorld>& Bullet3PhysicsSystem::getDynamicsWorld() const
  {
    return _world;
  }
  void Bullet3PhysicsSystem::update()
  {
    _world->stepSimulation(_gameTimer->getDeltaTimeSeconds(), _simulationSubSteps);
  }
}