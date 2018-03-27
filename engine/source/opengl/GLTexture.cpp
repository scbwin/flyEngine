#include <opengl/GLTexture.h>
#include <opengl/OpenGLUtils.h>

namespace fly
{
  GLTexture::GLTexture(GLenum target) : _target(target)
  {
    GL_CHECK(glGenTextures(1, &_id));
  }
  GLTexture::GLTexture(GLuint id, GLenum target) : _id(id), _target(target)
  {
  }
  void GLTexture::bind() const
  {
    GL_CHECK(glBindTexture(_target, _id));
  }
  GLTexture::~GLTexture()
  {
    GL_CHECK(glDeleteTextures(1, &_id));
  }
}