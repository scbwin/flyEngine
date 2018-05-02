#ifndef GLVERTEXARRAY_H
#define GLVERTEXARRAY_H

#include <GL/glew.h>

namespace fly
{
  class GLVertexArray
  {
  public:
    GLVertexArray();
    ~GLVertexArray();
    GLVertexArray(const GLVertexArray& other) = delete;
    GLVertexArray& operator=(const GLVertexArray& other) = delete;
    GLVertexArray(GLVertexArray&& other);
    GLVertexArray& operator=(GLVertexArray&& other);
    void bind() const;
  private:
    GLuint _id;
  };
}

#endif
