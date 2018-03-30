#include <opengl/GLBuffer.h>

namespace fly
{
  GLBuffer::GLBuffer(GLenum target) : _target(target)
  {
    GL_CHECK(glGenBuffers(1, &_id));
  }
  GLBuffer::~GLBuffer()
  {
    GL_CHECK(glDeleteBuffers(1, &_id));
  }
  void GLBuffer::bind() const
  {
    GL_CHECK(glBindBuffer(_target, _id));
  }
  void GLBuffer::bind(GLenum target) const
  {
    GL_CHECK(glBindBuffer(target, _id));
  }
}