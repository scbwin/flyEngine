#include <CameraController.h>
#include <Camera.h>

namespace fly
{
  CameraController::CameraController(const std::shared_ptr<Camera>& camera, float speed) : _camera(camera), _speed(speed)
  {
  }
  void CameraController::stepForward(float delta_time) const
  {
    _camera->_pos += _camera->_direction * delta_time * _speed;
  }
  void CameraController::stepBackward(float delta_time) const
  {
    _camera->_pos -= _camera->_direction * delta_time * _speed;
  }
  void CameraController::stepLeft(float delta_time) const
  {
    _camera->_pos -= _camera->_right * delta_time * _speed;
  }
  void CameraController::stepRight(float delta_time) const
  {
    _camera->_pos += _camera->_right * delta_time * _speed;
  }
  void CameraController::stepUp(float delta_time) const
  {
    _camera->_pos += _camera->_up * delta_time * _speed;
  }
  void CameraController::stepDown(float delta_time) const
  {
    _camera->_pos -= _camera->_up * delta_time * _speed;
  }
  void CameraController::mousePress(const Vec3f & mouse_pos)
  {
    _mousePos = mouse_pos;
    _pressed = true;
  }
  void CameraController::mouseMove(float delta_time, const Vec3f& mouse_pos)
  {
    if (_pressed) {
      auto delta = mouse_pos - _mousePos;
      _camera->_eulerAngles -= glm::vec3(delta) * delta_time;
    }
    _mousePos = mouse_pos;
  }
  void CameraController::mouseRelease()
  {
    _pressed = false;
  }
  void CameraController::setSpeed(float speed)
  {
    _speed = speed;
  }
}