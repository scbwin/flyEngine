#include <opengl/GLSampler.h>
#include <opengl/OpenGLUtils.h>

namespace fly
{
  GLSampler::GLSampler()
  {
    GL_CHECK(glGenSamplers(1, &_id));
  }
  GLSampler::~GLSampler()
  {
    GL_CHECK(glDeleteSamplers(1, &_id));
  }
  void GLSampler::bind(GLuint tex_unit) const
  {
    GL_CHECK(glBindSampler(tex_unit, _id));
  }
  void GLSampler::unbind(GLuint tex_unit) const
  {
    GL_CHECK(glBindSampler(tex_unit, 0));
  }
  void GLSampler::param(GLenum name, GLfloat param) const
  {
    GL_CHECK(glSamplerParameterf(_id, name, param));
  }
}