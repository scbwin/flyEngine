#include <PhysicsCameraController.h>
#include <GameTimer.h>
#include <Camera.h>
#include <iostream>

namespace fly
{
  PhysicsCameraController::PhysicsCameraController(const std::shared_ptr<Camera>& camera) : 
    _camera(camera),
    _currentPos(camera->getPosition()),
    _previousPos(camera->getPosition())
  {
  }
  void PhysicsCameraController::saveState()
  {
    _previousPos = _currentPos;
  }
  void PhysicsCameraController::updateSystem()
  {
    _velocity += _acceleration * _dt;
    _currentPos += _velocity * _dt;
    _velocity *= _damping;
  }
  void PhysicsCameraController::interpolate(float alpha)
  {
    _camera->setPosition(lerp(_previousPos, _currentPos, alpha));
  }
  void PhysicsCameraController::setAcceleration(const Vec3f & dir, float amount)
  {
    _acceleration = dir.length() > 0.f ? normalize(dir) * amount : 0.f;
  }
  const std::shared_ptr<Camera>& PhysicsCameraController::getCamera() const
  {
    return _camera;
  }
  void PhysicsCameraController::setDamping(float damping)
  {
    _damping = damping;
  }
}