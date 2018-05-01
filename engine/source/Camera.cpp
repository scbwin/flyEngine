#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace fly
{
  Camera::Camera(const Vec3f& pos, const Vec3f& euler_angles) : _pos(pos), _eulerAngles(euler_angles)
  {
  }

  Camera::~Camera()
  {
  }

  Mat4f Camera::getViewMatrix(const Vec3f& pos, const Vec3f& euler_angles)
  {
    _direction = Vec3f(cos(euler_angles[1]) * sin(euler_angles[0]), sin(euler_angles[1]), cos(euler_angles[1]) * cos(euler_angles[0]));
    _right = Vec3f(sin(euler_angles[0] - glm::half_pi<float>()), sin(euler_angles[2]), cos(euler_angles[0] - glm::half_pi<float>()));
    _up = cross(glm::vec3(_right), glm::vec3(_direction));

    Vec3f target = pos + _direction;

    return glm::lookAt(glm::vec3(pos), glm::vec3(target), glm::vec3(_up));
  }
  const Vec3f & Camera::getPosition() const
  {
    return _pos;
  }
  const Vec3f & Camera::getDirection() const
  {
    return _direction;
  }
  const Vec3f & Camera::getRight() const
  {
    return _right;
  }
  const Vec3f & Camera::getUp() const
  {
    return _up;
  }
  const Vec3f & Camera::getEulerAngles() const
  {
    return _eulerAngles;
  }
  void Camera::setPosition(const Vec3f & position)
  {
    _pos = position;
  }
  void Camera::setEulerAngles(const Vec3f & euler_angles)
  {
    _eulerAngles = euler_angles;
  }
  bool Camera::isActive() const
  {
    return _isActive;
  }
  void Camera::setActive(bool active)
  {
    _isActive = active;
  }
}