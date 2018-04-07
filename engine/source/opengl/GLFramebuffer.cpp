#include <opengl/GLFramebuffer.h>
#include <opengl/OpenGLUtils.h>
#include <opengl/GLTexture.h>

namespace fly
{
  GLFramebuffer::GLFramebuffer()
  {
    GL_CHECK(glGenFramebuffers(1, &_id));
  }
  GLFramebuffer::~GLFramebuffer()
  {
    GL_CHECK(glDeleteFramebuffers(1, &_id));
  }
  void GLFramebuffer::bind() const
  {
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, _id));
  }
  void GLFramebuffer::texture2D(GLenum attachment, const GLTexture & tex, GLint level) const
  {
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, attachment, tex.id(), level));
  }
}