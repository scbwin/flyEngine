#ifndef GLAPPENDBUFFER_H
#define GLAPPENDBUFFER_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <memory>
#include <opengl/GLBuffer.h>

namespace fly
{
  class GLAppendBuffer
  {
  public:
    GLAppendBuffer(GLenum target);
    template<typename T>
    void appendData(T const * data, size_t num_elements)
    {
      _buffer.bind(GL_COPY_READ_BUFFER);
      int num_bytes_old;
      GL_CHECK(glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &num_bytes_old));
      GLBuffer buffer_new(_target);
      buffer_new.setData<unsigned char>(nullptr, num_bytes_old + num_elements * sizeof(T));
      buffer_new.bind(GL_COPY_WRITE_BUFFER);
      GL_CHECK(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, num_bytes_old)); // Copy old content to new buffer
      GL_CHECK(glBufferSubData(GL_COPY_WRITE_BUFFER, num_bytes_old, num_elements * sizeof(T), data)); // Append new content
      _buffer = std::move(buffer_new); // Switch buffers
    }
    const GLBuffer& getBuffer() const;
  private:
    GLenum _target;
    GLBuffer _buffer;
  };
}

#endif
