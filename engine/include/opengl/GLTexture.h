#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#include <GL/glew.h>
#include <math/FlyMath.h>

namespace fly
{
  class GLTexture
  {
  public:
    GLTexture(GLenum target = GL_TEXTURE_2D);
    GLTexture(GLuint id, GLenum target = GL_TEXTURE_2D);
    GLuint id() const;
    void bind() const;
    void image2D(GLint level, GLint internal_format, const Vec2u& size, GLint border, GLenum format, GLenum type, const void* data);
    void param(GLenum name, GLint val) const;
    ~GLTexture();
  private:
    GLuint _id;
    GLenum _target;
    unsigned _width;
    unsigned _height;
  };
}

#endif // !GLTEXTURE_H
