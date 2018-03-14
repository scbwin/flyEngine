#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include "Component.h"

namespace fly
{
  class Transform : public Component
  {
  public:
    Transform(const glm::vec3& translation = glm::vec3(0.f), const glm::vec3& scale = glm::vec3(1.f), const glm::vec3& degrees = glm::vec3(0.f));
    glm::mat4 getModelMatrix() const;
    glm::vec3& getTranslation();
    glm::vec3& getScale();
  private:
    glm::vec3 _translation;
    glm::vec3 _scale;
    glm::vec3 _degrees;


  };
}

#endif // !TRANSFORM_H
