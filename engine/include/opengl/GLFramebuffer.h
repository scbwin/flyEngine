#ifndef GLFRAMEBUFFER_H
#define GLFRAMEBUFFER_H

#include <GL/glew.h>
#include <memory>
#include <unordered_set>

namespace fly
{
  class GLTexture;

  class GLFramebuffer
  {
  public:
    GLFramebuffer();
    ~GLFramebuffer();
    void bind() const;
    void texture(GLenum attachment, const std::shared_ptr<GLTexture>& tex, GLint level);
    void clearAttachments();
  private:
    GLuint _id;
    std::unordered_set<GLenum> _attachments;
  };
}

#endif
