#include <Billboard.h>

namespace fly
{
  Billboard::Billboard(const std::string& texture_path, float opacity, const glm::vec2& size) : _texturePath(texture_path), _opacity(opacity), _size(size)
  {
  }
  std::string & Billboard::getTexturePath()
  {
    return _texturePath;
  }
  float Billboard::getOpacity()
  {
    return _opacity;
  }
  glm::vec2 Billboard::getSize()
  {
    return _size;
  }
}