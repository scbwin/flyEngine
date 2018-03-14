#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <Component.h>
#include <string>
#include <glm/glm.hpp>

namespace fly
{
  class Billboard : public Component {
  public:
    Billboard(const std::string& texture_path, float opacity = 1.f, const glm::vec2& size = glm::vec2(1.f));
    std::string& getTexturePath();
    float getOpacity();
    glm::vec2 getSize();
  private:
    std::string _texturePath;
    float _opacity;
    glm::vec2 _size;
  };
}

#endif // !BILLBOARD_H
