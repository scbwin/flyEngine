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
  GLTexture::GLTexture(const GLTexture & other) : 
    _target(other._target)
  {
    GL_CHECK(glGenTextures(1, &_id));
    init(other);
    copy(other);
  }
  GLTexture & GLTexture::operator=(const GLTexture & other)
  {
    if (_width != other._width || _height != other._height || _depth != other._depth) {
      init(other);
    }
    copy(other);
    return *this;
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
    _depth = 1;
    _format = format;
    _internalFormat = internal_format;
    _type = type;
  }
  void GLTexture::image3D(GLint level, GLint internal_format, const Vec3u & size, GLint border, GLenum format, GLenum type, const void * data)
  {
    bind();
    GL_CHECK(glTexImage3D(_target, level, internal_format, size[0], size[1], size[2], border, format, type, data));
    _width = size[0];
    _height = size[1];
    _depth = size[2];
    _format = format;
    _internalFormat = internal_format;
    _type = type;
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
  unsigned GLTexture::depth() const
  {
    return _depth;
  }
  GLuint GLTexture::format() const
  {
    return _format;
  }
  GLenum GLTexture::target() const
  {
    return _target;
  }
  GLTexture::~GLTexture()
  {
    GL_CHECK(glDeleteTextures(1, &_id));
  }
  void GLTexture::init(const GLTexture & other)
  {
    if (other._depth > 1) {
      image3D(0, other._internalFormat, Vec3u(other._width, other._height, other._depth), 0, other._format, other._type, nullptr);
    }
    else {
      image2D(0, other._internalFormat, Vec2u(other._width, other._height), 0, other._format, other._type, nullptr);
    }
  }
  void GLTexture::copy(const GLTexture & other)
  {
    param(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GL_CHECK(glCopyImageSubData(other._id, other._target, 0, 0, 0, 0, _id, _target, 0, 0, 0, 0, other._width, other._height, other._depth));
  }
}