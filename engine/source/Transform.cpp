#include <Transform.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace fly
{
  Transform::Transform(const Vec3f & translation, const Vec3f & scale, const Vec3f & degrees) : _translation(translation), _scale(scale), _degrees(degrees)
  {
  }
  Mat4f Transform::getModelMatrix() const
  {
    auto translation_matrix = translate<4, float>(_translation);
    auto scale_matrix = scale<4, float>(_scale);
    Mat4f rotation_matrix_x = glm::rotate(glm::radians(_degrees[0]), glm::vec3(1.f, 0.f, 0.f));
    Mat4f rotation_matrix_y = glm::rotate(glm::radians(_degrees[1]), glm::vec3(0.f, 1.f, 0.f));
    Mat4f rotation_matrix_z = glm::rotate(glm::radians(_degrees[2]), glm::vec3(0.f, 0.f, 1.f));

    return translation_matrix * rotation_matrix_x * rotation_matrix_z * rotation_matrix_y * scale_matrix;
  }
  void Transform::setTranslation(const Vec3f & translation)
  {
    _translation = translation;
  }
  void Transform::setScale(const Vec3f & scale)
  {
    _scale = _scale;
  }
  void Transform::setDegrees(const Vec3f & degrees)
  {
    _degrees = degrees;
  }
  const Vec3f& Transform::getTranslation() const
  {
    return _translation;
  }
  const Vec3f& Transform::getScale() const
  {
    return _scale;
  }
  const Vec3f & Transform::getDegrees() const
  {
    return _degrees;
  }
}