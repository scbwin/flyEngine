#include "Transform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace fly
{
  Transform::Transform(const glm::vec3 & translation, const glm::vec3 & scale, const glm::vec3 & degrees) : _translation(translation), _scale(scale), _degrees(degrees)
  {
  }
  glm::mat4 Transform::getModelMatrix() const
  {
    auto translation_matrix = glm::translate(_translation);
    auto scale_matrix = glm::scale(_scale);
    auto rotation_matrix_x = glm::rotate(glm::radians(_degrees.x), glm::vec3(1.f, 0.f, 0.f));
    auto rotation_matrix_y = glm::rotate(glm::radians(_degrees.y), glm::vec3(0.f, 1.f, 0.f));
    auto rotation_matrix_z = glm::rotate(glm::radians(_degrees.z), glm::vec3(0.f, 0.f, 1.f));

    return translation_matrix * rotation_matrix_x * rotation_matrix_z * rotation_matrix_y * scale_matrix;
  }
  glm::vec3& Transform::getTranslation()
  {
    return _translation;
  }
  glm::vec3& Transform::getScale()
  {
    return _scale;
  }
}