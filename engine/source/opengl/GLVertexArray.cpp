#include <opengl/GLVertexArray.h>
#include <opengl/OpenGLUtils.h>

namespace fly
{
  GLVertexArray::GLVertexArray()
  {
    GL_CHECK(glGenVertexArrays(1, &_id));
  }
  GLVertexArray::~GLVertexArray()
  {
    GL_CHECK(glDeleteVertexArrays(1, &_id));
  }
  void GLVertexArray::bind() const
  {
    GL_CHECK(glBindVertexArray(_id));
  }
}