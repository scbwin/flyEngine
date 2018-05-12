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
    void link() const;
    void bind() const;
    inline GLint uniformLocation(const std::string& name) { return _uniformLocations.getOrCreate(name, name); }
    const std::vector<GLShaderSource>& getSources() const;
  private:
    GLuint _id;
    std::vector<GLShaderSource> _sources;
    SoftwareCache<std::string, GLint, const std::string&> _uniformLocations;
  };
}

#endif
