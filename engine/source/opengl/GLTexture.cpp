#include <opengl/GLTexture.h>
#include <opengl/OpenGLUtils.h>

namespace fly
{
  GLTexture::GLTexture(GLenum target) : _target(target)
  {
    GL_CHECK(glGenTextures(1, &_id));
  }
  GLTexture::GLTexture(GLuint id, GLenum target) : _id(id), _target(target)
  {
  }
  GLuint GLTexture::id() const
  {
    return _id;
  }
  void GLTexture::bind() const
  {
    GL_CHECK(glBindTexture(_target, _id));
  }
  void GLTexture::image2D(GLint level, GLint internal_format, const Vec2u & size, GLint border, GLenum format, GLenum type, const void * data)
  {
    bind();
    GL_CHECK(glTexImage2D(_target, level, internal_format, size[0], size[1], border, format, type, data));
    _width = size[0];
    _height = size[1];
  }
  void GLTexture::param(GLenum name, GLint val) const
  {
    bind();
    GL_CHECK(glTexParameteri(_target, name, val));
  }
  void GLTexture::param(GLenum name, const GLfloat * val) const
  {
    bind();
    GL_CHECK(glTexParameterfv(_target, name, val));
  }
  unsigned GLTexture::width() const
  {
    return _width;
  }
  unsigned GLTexture::height() const
  {
    return _height;
  }
  GLTexture::~GLTexture()
  {
    GL_CHECK(glDeleteTextures(1, &_id));
  }
}