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
  void GLFramebuffer::texture(GLenum attachment, const std::shared_ptr<GLTexture>& tex, GLint level)
  {
    if (tex) {
      _attachments.insert(attachment);
    }
    GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, attachment, tex ? tex->id() : 0, level));
  }
  void GLFramebuffer::textureLayer(GLenum attachment, const std::shared_ptr<GLTexture>& tex, GLint level, GLint layer)
  {
    if (tex) {
      _attachments.insert(attachment);
    }
    GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, tex ? tex->id() : 0, level, layer));
  }
  void GLFramebuffer::clearAttachments()
  {
    for (const auto& a : _attachments) {
      GL_CHECK(glFramebufferTexture(GL_FRAMEBUFFER, a, 0, 0));
    }
    _attachments.clear();
  }
}