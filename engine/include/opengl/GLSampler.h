#ifndef GLSAMPLER_H
#define GLSAMPLER_H

#include <GL/glew.h>

namespace fly
{
  class GLSampler
  {
  public:
    GLSampler();
    ~GLSampler();
    void param(GLenum name, GLfloat param) const;
    void bind(GLuint tex_unit) const;
    void unbind(GLuint tex_unit) const;
  private:
    GLuint _id;
  };
}

#endif
