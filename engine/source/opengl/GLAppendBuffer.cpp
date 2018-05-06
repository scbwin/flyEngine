#include <opengl/GLAppendBuffer.h>

namespace fly
{
  GLAppendBuffer::GLAppendBuffer(GLenum target) : _target(target), _buffer(target)
  {
  }
  const GLBuffer& GLAppendBuffer::getBuffer() const
  {
    return _buffer;
  }
}