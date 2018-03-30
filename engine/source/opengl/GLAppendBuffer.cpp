#include <opengl/GLAppendBuffer.h>

namespace fly
{
  GLAppendBuffer::GLAppendBuffer(GLenum target) : _target(target)
  {
  }
  const std::shared_ptr<GLBuffer>& GLAppendBuffer::getBuffer() const
  {
    return _buffer;
  }
}