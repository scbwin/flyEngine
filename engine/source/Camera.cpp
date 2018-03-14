#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace fly
{
  Camera::Camera(const glm::vec3& pos, const glm::vec3& euler_angles) : _pos(pos), _eulerAngles(euler_angles)
  {
  }

  Camera::~Camera()
  {
  }

  glm::mat4 Camera::getViewMatrix(const glm::vec3& pos, const glm::vec3& euler_angles)
  {
    _direction = glm::vec3(cos(euler_angles.y) * sin(euler_angles.x), sin(euler_angles.y), cos(euler_angles.y) * cos(euler_angles.x));
    _right = glm::vec3(sin(euler_angles.x - glm::half_pi<float>()), sin(euler_angles.z), cos(euler_angles.x - glm::half_pi<float>()));
    _up = cross(_right, _direction);

    glm::vec3 target = pos + _direction;

    return glm::lookAt(pos, target, _up);
  }
}