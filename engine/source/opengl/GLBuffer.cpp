#include <opengl/GLBuffer.h>
#include <iostream>

namespace fly
{
  GLBuffer::GLBuffer()
  {
  }
  GLBuffer::GLBuffer(GLenum target) : _target(target)
  {
    GL_CHECK(glGenBuffers(1, &_id));
  }
  GLBuffer::~GLBuffer()
  {
    if (_id) {
      GL_CHECK(glDeleteBuffers(1, &_id));
    }
  }
  GLBuffer::GLBuffer(GLBuffer && other) :
    _id(other._id),
    _target(other._target)
  {
    other._id = 0;
  }
  GLBuffer & GLBuffer::operator=(GLBuffer && other)
  {
    if (_id) {
      GL_CHECK(glDeleteBuffers(1, &_id));
    }
    _id = other._id;
    _target = other._target;
    other._id = 0;
    return *this;
  }
  void GLBuffer::bind() const
  {
    GL_CHECK(glBindBuffer(_target, _id));
  }
  void GLBuffer::bind(GLenum target) const
  {
    GL_CHECK(glBindBuffer(target, _id));
  }
  void GLBuffer::bindBase(unsigned index) const
  {
    GL_CHECK(glBindBufferBase(_target, index, _id));
  }
  void GLBuffer::bindBase(GLenum target, unsigned index) const
  {
    GL_CHECK(glBindBufferBase(target, index, _id));
  }
  void GLBuffer::unmap() const
  {
    GL_CHECK(glUnmapBuffer(_target));
  }
}