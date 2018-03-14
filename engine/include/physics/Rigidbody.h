#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include <glm/glm.hpp>

namespace fly
{
  struct Rigidbody
  {
    glm::vec3 _pos;
    glm::vec3 _velocity;
    glm::vec3 _impulse;
  };
}

#endif