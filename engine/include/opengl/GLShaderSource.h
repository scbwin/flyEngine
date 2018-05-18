#ifndef GLSHADERSOURCE_H
#define GLSHADERSOURCE_H

#include <string>
#include <GL/glew.h>

namespace fly
{
  struct GLShaderSource
  {
    GLShaderSource() = default;
    GLShaderSource(const std::string& file, GLenum type);
    std::string _key;
    std::string _source;
    GLenum _type;
    GLuint _id;
  };
}

#endif
