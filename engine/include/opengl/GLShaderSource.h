#ifndef GLSHADERSOURCE_H
#define GLSHADERSOURCE_H

#include <string>
#include <GL/glew.h>

namespace fly
{
  struct GLShaderSource
  {
    std::string _key;
    std::string _source;
    GLenum _type;
    GLuint _id;
    void initFromFile(const std::string& file, GLenum type);
  };
}

#endif
