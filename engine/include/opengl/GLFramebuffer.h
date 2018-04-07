#ifndef GLFRAMEBUFFER_H
#define GLFRAMEBUFFER_H

#include <GL/glew.h>

namespace fly
{
  class GLTexture;

  class GLFramebuffer
  {
  public:
    GLFramebuffer();
    ~GLFramebuffer();
    void bind() const;
    void texture2D(GLenum attachment, const GLTexture& tex, GLint level) const;
  private:
    GLuint _id;
  };
}

#endif
