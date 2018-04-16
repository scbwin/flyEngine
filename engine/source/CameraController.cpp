#include <CameraController.h>
#include <Camera.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

namespace fly
{
  CameraController::CameraController(const std::shared_ptr<Camera>& camera, float speed) : _camera(camera), _speed(speed)
  {
  }
  void CameraController::stepForward(float delta_time) const
  {
    _camera->_pos += _camera->_direction * delta_time * _speed * _accelerate;
  }
  void CameraController::stepBackward(float delta_time) const
  {
    _camera->_pos -= _camera->_direction * delta_time * _speed *_accelerate;
  }
  void CameraController::stepLeft(float delta_time) const
  {
    _camera->_pos -= _camera->_right * delta_time * _speed * _accelerate;
  }
  void CameraController::stepRight(float delta_time) const
  {
    _camera->_pos += _camera->_right * delta_time * _speed * _accelerate;
  }
  void CameraController::stepUp(float delta_time) const
  {
    _camera->_pos += _camera->_up * delta_time * _speed * _accelerate;
  }
  void CameraController::stepDown(float delta_time) const
  {
    _camera->_pos -= _camera->_up * delta_time * _speed * _accelerate;
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
  }
  void CameraController::mousePress(const Vec3f & mouse_pos)
  {
    _mousePos = mouse_pos;
    _pressed = true;
  }
  void CameraController::mouseMove(const Vec3f& mouse_pos)
  {
    if (_pressed) {
      auto delta = mouse_pos - _mousePos;
      _camera->_eulerAngles -= glm::vec3(delta) * _mouseSpeed;
    }
    _mousePos = mouse_pos;
     const float eps = 0.01f;
    _camera->_eulerAngles.y = glm::clamp(_camera->_eulerAngles.y, -glm::half_pi<float>() + eps, glm::half_pi<float>() - eps);
  }
  void CameraController::mouseRelease()
  {
    _pressed = false;
  }
  void CameraController::setSpeed(float speed)
  {
    _speed = speed;
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
}