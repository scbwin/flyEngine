#include <CameraController.h>
#include <Camera.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace fly
{
  CameraController::CameraController(const std::shared_ptr<Camera>& camera, float speed) : _camera(camera), _baseSpeed(speed)
  {
  }
  void CameraController::setCamera(const std::shared_ptr<Camera>& camera)
  {
    _camera = camera;
  }
 /* void CameraController::stepForward(float delta_time) const
  {
    updatePos(_camera->getDirection(), delta_time);
  }
  void CameraController::stepBackward(float delta_time) const
  {
    updatePos(_camera->getDirection() * -1.f, delta_time);
  }
  void CameraController::stepLeft(float delta_time) const
  {
    updatePos(_camera->getRight() * -1.f, delta_time);
  }
  void CameraController::stepRight(float delta_time) const
  {
    updatePos(_camera->getRight(), delta_time);
  }
  void CameraController::stepUp(float delta_time) const
  {
    updatePos(_camera->getUp(), delta_time);
  }
  void CameraController::stepDown(float delta_time) const
  {
    updatePos(_camera->getUp() * -1.f, delta_time);
  }
  void CameraController::acceleratePressed()
  {
    _accelerate = 2.f;
  }
  void CameraController::accelerateReleased()
  {
    _accelerate = 1.f;
  }
  void CameraController::deceleratePressed()
  {
    _accelerate = 0.5f;
  }
  void CameraController::decelerateReleased()
  {
    _accelerate = 1.f;
  }*/
  void CameraController::mousePress(const Vec3f & mouse_pos)
  {
    _mousePos = mouse_pos;
    _pressed = true;
  }
  void CameraController::mouseMove(const Vec3f& mouse_pos)
  {
    if (_pressed) {
      auto delta = mouse_pos - _mousePos;
      _camera->setEulerAngles( _camera->getEulerAngles() - delta * _mouseSpeed);
    }
    _mousePos = mouse_pos;
     const float eps = 0.01f;
     auto euler_angles = _camera->getEulerAngles();
     euler_angles[1] = glm::clamp(euler_angles[1], -glm::half_pi<float>() + eps, glm::half_pi<float>() - eps);
     _camera->setEulerAngles(euler_angles);
  }
  void CameraController::mouseRelease()
  {
    _pressed = false;
  }
  void CameraController::setSpeed(float speed)
  {
    _baseSpeed = speed;
  }
  float CameraController::getSpeed() const
  {
    return _baseSpeed;
  }
  bool CameraController::isPressed() const
  {
    return _pressed;
  }
  float CameraController::getMouseSpeed() const
  {
    return _mouseSpeed;
  }
  void CameraController::setMouseSpeed(float speed)
  {
    _mouseSpeed = speed;
  }
  const std::shared_ptr<Camera>& CameraController::getCamera() const
  {
    return _camera;
  }
  void CameraController::setSpeedFactor(float speed_factor)
  {
    _speedFactor = speed_factor;
  }
  float CameraController::getSpeedFactor() const
  {
    return _speedFactor;
  }
  void CameraController::updatePos(const Vec3f & dir, float delta_time) const
  {
    _camera->setPosition(_camera->getPosition() + dir * delta_time * _baseSpeed * _accelerate * _speedFactor);
  }
}