#ifndef GLWRAPPERS_H
#define GLWRAPPERS_H

#include "GL/glew.h"
#include <string>
#include <functional>
#include <set>
#include <memory>
#include <vector>
#include "OpenGLUtils.h"
#include <map>
#include <regex>
#include <unordered_map>
#include <SoftwareCache.h>

namespace fly
{
  class GLVertexArrayOld
  {
  public:
    void create();
    void bind();
    virtual ~GLVertexArrayOld();
  private:
    GLuint _id;
  };

  class GLShaderProgram
  {
  public:
    enum class ShaderType
    {
      VERTEX, FRAGMENT, COMPUTE, GEOMETRY
    };
    GLShaderProgram();
    ~GLShaderProgram();
    GLShaderProgram(const GLShaderProgram& other) = delete;
    GLShaderProgram& operator=(const GLShaderProgram& other) = delete;
    void create();
    void addShaderFromFile(const std::string& fname, ShaderType type);
    void link();
    void bind() const;
    GLuint id() const;
    GLint uniformLocation(const std::string& name);
    std::vector<std::string> _fnames;
    std::vector<ShaderType> _types;
    std::vector<GLuint> _shaders;
   // std::unordered_map<std::string, GLint> _uniformLocations;
    SoftwareCache<std::string, GLint, const std::string&> _uniformLocations;
    void reload();
    const std::vector<std::string>& getFileNames() const;
  private:
    GLuint _id;
    void add(const std::string& fname, ShaderType type);
  };

  class GLBufferOld
  {
  public:
    void create(GLenum target = GL_ARRAY_BUFFER);
    void setData(const void* data, size_t size_in_bytes);
    void bind();
    virtual ~GLBufferOld();
  private:
    GLuint _id;
    GLenum _target;
  };

  class GLTextureOld
  {
  public:
    GLTextureOld(GLuint id, GLenum target);
    GLTextureOld(GLenum target);
    void create();
    void bind();
    GLuint id();
    void setMinificationFilter(GLint filter);
    void setMagnificationFilter(GLint filter);
    void setCompareMode(GLenum mode, GLenum func);
    void setData(int width, int height, GLint internal_format, GLenum format, GLenum type, void* data);
    void setData(int width, int height, int depth, GLint internal_format, GLenum format, GLenum type, void* data);
    void texStorage2D(int width, int height, GLint internal_format, int levels = 1);
    void texStorage3D(int width, int height, int depth, GLint internal_format, int levels = 1);
    virtual ~GLTextureOld();
    int width();
    int height();
    int depth();
  private:
    GLenum _target;
    GLuint _id;
    int _width;
    int _height;
    int _depth;
  };

  struct GLRenderbuffer
  {
    GLuint _id;
    void create();
    void bind();
    void allocate(GLenum internal_format, int width, int height);
    virtual ~GLRenderbuffer();
  };

  class GLFramebufferOld
  {
  public:
    void create(int width, int height);
    void bind();
    GLuint id();
    int width();
    int height();
    void setViewport();
    void setDepthTexture(const std::shared_ptr<GLTextureOld>& depth_texture);
    void addTexture(const std::shared_ptr<GLTextureOld>& texture, GLenum attachment);
    void addTextureLayer(const std::shared_ptr<GLTextureOld>& texture, GLenum attachment, int layer);
    void setRenderbuffer(const std::shared_ptr<GLRenderbuffer>& renderbuffer, GLenum attachment);
    void setDrawbuffers(const std::vector<GLenum>& draw_buffers);
    void setDrawbuffersFromColorAttachments();
    void checkStatus();
    std::vector<std::shared_ptr<GLTextureOld>>& textures();
    virtual ~GLFramebufferOld();
  private:
    GLuint _id;
    int _width;
    int _height;
    std::vector<std::shared_ptr<GLTextureOld>> _textures;
    std::vector<GLenum> _colorAttachments;
    std::shared_ptr<GLRenderbuffer> _renderbuffer;
    std::shared_ptr<GLTextureOld> _depthBuffer;
  };

  class GLQuery
  {
  public:
    GLQuery(GLenum target);
    virtual ~GLQuery();
    void begin();
    void end();
    int getResult();
  private:
    GLuint _id;
    GLenum _target;
    int _ready = 1;
    int _result = 0;
  };

  struct GLShaderStorageBuffer
  {
    unsigned int _id;
    unsigned int _numElements = 0;
    GLShaderStorageBuffer();
    ~GLShaderStorageBuffer();
    void bindBufferBase(int index);

    template <class T>
    void setData(T* data, unsigned int num_elements)
    {
      _numElements = num_elements;
      GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id));
      GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, num_elements * sizeof(T), data, GL_STATIC_DRAW));
    }

    template <class T>
    void readDataFromGPU(T* data, unsigned int offset, unsigned int length)
    {
      GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id));
      auto ptr = reinterpret_cast<T*>(glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset * sizeof(T), length * sizeof(T), GL_MAP_READ_BIT));
      std::copy(ptr, ptr + length, data);
      glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

  };
}

#endif
