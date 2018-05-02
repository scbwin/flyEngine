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
    GLFramebuffer(const GLFramebuffer& other) = delete;
    GLFramebuffer& operator=(const GLFramebuffer& other) = delete;
    void bind() const;
    void texture(GLenum attachment, const GLTexture* tex, GLint level);
    void textureLayer(GLenum attachment, const GLTexture* tex, GLint level, GLint layer);
    void clearAttachments();
  private:
    GLuint _id;
    std::unordered_set<GLenum> _attachments;
  };
}

#endif
