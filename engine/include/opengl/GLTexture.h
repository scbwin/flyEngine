#ifndef GLTEXTURE_H
#define GLTEXTURE_H

#include <GL/glew.h>
#include <math/FlyMath.h>

namespace fly
{
  class GLTexture
  {
  public:
    GLTexture(GLenum target);
    GLTexture(GLuint id, GLenum target);
    GLTexture(const GLTexture& other);
    GLTexture& operator=(const GLTexture& other) = delete;
    GLuint id() const;
    void bind() const;
    void image2D(GLint level, GLint internal_format, const Vec2u& size, GLint border, GLenum format, GLenum type, const void* data);
    void image3D(GLint level, GLint internal_format, const Vec3u& size, GLint border, GLenum format, GLenum type, const void* data);
    void param(GLenum name, GLint val) const;
    void param(GLenum name, const GLfloat* val) const;
    unsigned width() const;
    unsigned height() const;
    unsigned depth() const;
    GLuint format() const;
    ~GLTexture();
  private:
    GLuint _id;
    GLenum _target;
    unsigned _width;
    unsigned _height;
    unsigned _depth = 1;
    GLuint _internalFormat;
    GLuint _format;
    GLuint _type;
  };
}

#endif // !GLTEXTURE_H
