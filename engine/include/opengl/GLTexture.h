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
    GLTexture& operator=(const GLTexture& other);
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
    GLenum target() const;
    ~GLTexture();
  private:
    GLuint _id;
    GLenum _target;
    unsigned _width = 0;
    unsigned _height = 0;
    unsigned _depth = 0;
    GLuint _internalFormat;
    GLuint _format;
    GLuint _type;
    void init(const GLTexture& other);
    void copy(const GLTexture& other);
  };
}

#endif // !GLTEXTURE_H
