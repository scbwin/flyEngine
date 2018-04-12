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
    template<typename T> void setData(const T* data, size_t num_elements, GLenum usage = GL_STATIC_DRAW) const
    {
      bind();
      GL_CHECK(glBufferData(_target, num_elements * sizeof(T), data, usage));
    }
    template<typename T> T* map(GLenum access) const
    {
      T* ptr;
      GL_CHECK(ptr = reinterpret_cast<T*>(glMapBuffer(_target, access)));
      return ptr;
    }
    void unmap() const;
  private:
    GLuint _id;
    GLenum _target;
  };
}

#endif 
