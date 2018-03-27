#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#include <GL/glew.h>

namespace fly
{
  class GLTexture
  {
  public:
    GLTexture(GLenum target = GL_TEXTURE_2D);
    GLTexture(GLuint id, GLenum target = GL_TEXTURE_2D);
    void bind() const;
    ~GLTexture();
  private:
    GLuint _id;
    GLenum _target;
    int _width;
    int _height;
  };
}

#endif // !GLTEXTURE_H
