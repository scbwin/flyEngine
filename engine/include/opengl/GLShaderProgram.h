#ifndef GLSHADERPROGRAM_H
#define GLSHADERPROGRAM_H

#include <GL/glew.h>
#include <opengl/GLShaderSource.h>
#include <vector>
#include <SoftwareCache.h>

namespace fly
{
  class GLShaderProgram
  {
  public:
    GLShaderProgram();
    ~GLShaderProgram();
    GLShaderProgram(const GLShaderProgram& other) = delete;
    GLShaderProgram& operator=(const GLShaderProgram& other) = delete;
    GLShaderProgram(GLShaderProgram&& other);
    GLShaderProgram& operator=(GLShaderProgram&& other);
    void add(GLShaderSource& source);
    void link();
    void bind() const;
    inline const GLint & uniformLocation(const char* name) const
    {
      return _uniformLocations.at(name);
    }
    const std::vector<GLShaderSource>& getSources() const;
  private:
    struct Comparator
    {
      inline bool operator()(const char* a, const char* b) const
      {
        return std::strcmp(a, b) < 0;
      }
    };
    std::map<const char*, GLint, Comparator> _uniformLocations;
    GLuint _id;
    std::vector<char*> _uniformNames;
    std::vector<GLShaderSource> _sources;
    void cleanup();

  };
}

#endif
