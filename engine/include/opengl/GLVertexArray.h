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
    void bind() const;
  private:
    GLuint _id;
  };
}

#endif
