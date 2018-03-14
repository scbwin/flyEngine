#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"
#include "Component.h"

namespace fly
{
  class Camera : public Component
  {
  public:
    Camera(const glm::vec3& pos, const glm::vec3& euler_angles);
    virtual ~Camera();
    glm::mat4 getViewMatrix(const glm::vec3& pos, const glm::vec3& euler_angles);

    glm::vec3 _pos;
    glm::vec3 _eulerAngles;
    glm::vec3 _right;
    glm::vec3 _direction;
    glm::vec3 _up;
    bool _isActive = true;
  };
}

#endif