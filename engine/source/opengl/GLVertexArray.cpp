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
    if (_id) {
      GL_CHECK(glDeleteVertexArrays(1, &_id));
    }
  }
  GLVertexArray::GLVertexArray(GLVertexArray && other)
  {
    _id = other._id;
    other._id = 0;
  }
  GLVertexArray & GLVertexArray::operator=(GLVertexArray && other)
  {
    if (_id) {
      GL_CHECK(glDeleteVertexArrays(1, &_id));
    }
    _id = other._id;
    other._id = 0;
    return *this;
  }
  void GLVertexArray::bind() const
  {
    GL_CHECK(glBindVertexArray(_id));
  }
}