#ifndef GLVERTEXBUFFER_H
#define GLVERTEXBUFFER_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>

namespace fly
{
  class GLBuffer
  {
  public:
    GLBuffer(GLenum target);
    ~GLBuffer();
    void bind() const;
    void bind(GLenum target) const;
    template<typename T>
    void setData(const T* data, size_t num_elements, GLenum usage = GL_STATIC_DRAW) const
    {
      bind();
      GL_CHECK(glBufferData(_target, num_elements * sizeof(T), data, usage));
    }
  private:
    GLuint _id;
    GLenum _target;
  };
}

#endif 
