#ifndef GLBYTEBUFFERSTACK_H
#define GLBYTEBUFFERSTACK_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <memory>
#include <opengl/GLBuffer.h>
#include <iostream>

namespace fly
{
  class GLByteBufferStack
  {
  public:
    GLByteBufferStack(GLenum target) :
      GLByteBufferStack(target, 1)
    {
    }
    GLByteBufferStack(GLenum target, size_t init_size_bytes) :
      _target(target), _buffer(target)
    {
      reserve<unsigned char>(init_size_bytes);
    }
    template<typename T>
    inline void push_back(T const* data, size_t num_elements)
    {
      size_t bytes_to_append = num_elements * sizeof(T);
      size_t new_size = _size + bytes_to_append;
      while (_capacity < new_size) {
        reserve<unsigned char>(_capacity * 2);
      }
      _buffer.bind(GL_COPY_WRITE_BUFFER);
      GL_CHECK(glBufferSubData(GL_COPY_WRITE_BUFFER, _size, bytes_to_append, data));
      _size = new_size;
    }
    inline const GLBuffer& getBuffer() const
    {
      return _buffer;
    }
    template<typename T>
    inline void reserve(size_t new_size)
    {
      assert(new_size != 0);
      _capacity = new_size * sizeof(T);
      GLBuffer buffer(_target);
      buffer.setData<unsigned char>(nullptr, _capacity);
      if (_size) {
        buffer.bind(GL_COPY_WRITE_BUFFER);
        _buffer.bind(GL_COPY_READ_BUFFER);
        GL_CHECK(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, _size)); // Copy old content to the new buffer
      }
      _buffer = std::move(buffer);
    }
  private:
    GLenum _target;
    GLBuffer _buffer;
    size_t _size = 0; // in bytes
    size_t _capacity = 0; // in bytes
  };
}

#endif
