#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLUtils.h>
#include <iostream>

namespace fly
{
  GLShaderProgram::GLShaderProgram() : 
    _uniformLocations(SoftwareCache<std::string, GLint, const std::string&>([this](const std::string& name) {
    GLint loc;
    GL_CHECK(loc = glGetUniformLocation(_id, name.c_str()));
#ifdef _DEBUG
    if (loc == -1) {
      std::string err;
      for (const auto& s : _sources) {
        err += s._key + ",";
      }
      err += " No valid uniform location for name: " + name;
      throw std::exception(err.c_str());
    }
#endif
    return loc;
  }))
  {
    GL_CHECK(_id = glCreateProgram());
  }
  GLShaderProgram::~GLShaderProgram()
  {
    if (_id) {
      GL_CHECK(glDeleteProgram(_id));
    }
  }
  GLShaderProgram::GLShaderProgram(GLShaderProgram && other) :
    _id(other._id),
    _sources(other._sources),
    _uniformLocations(other._uniformLocations)
  {
    other._id = 0;
  }
  GLShaderProgram & GLShaderProgram::operator=(GLShaderProgram && other)
  {
    if (_id) {
      GL_CHECK(glDeleteProgram(_id));
    }
    _id = other._id;
    _sources = std::move(other._sources);
    _uniformLocations = other._uniformLocations;
    other._id = 0;
    return *this;
  }
  void GLShaderProgram::add(GLShaderSource& source)
  {
    GL_CHECK(source._id = glCreateShader(source._type));
    auto string = source._source.c_str();
    GL_CHECK(glShaderSource(source._id, 1, &string, nullptr));
    GL_CHECK(glCompileShader(source._id));
    int compile_status, log_length;
    GL_CHECK(glGetShaderiv(source._id, GL_COMPILE_STATUS, &compile_status));
    GL_CHECK(glGetShaderiv(source._id, GL_INFO_LOG_LENGTH, &log_length));
    if (compile_status == GL_FALSE) {
      std::string message(log_length + 1, ' ');
      GL_CHECK(glGetShaderInfoLog(source._id, log_length, NULL, &message[0]));
      std::cout << "Error in " << source._key << std::endl;
      std::cout << string << std::endl;
      std::cout << message << std::endl;
      return;
    }
    GL_CHECK(glAttachShader(_id, source._id));
    _sources.push_back(source);
  }
  void GLShaderProgram::link() const
  {
    GL_CHECK(glLinkProgram(_id));
    GLint linkStatus;
    GL_CHECK(glGetProgramiv(_id, GL_LINK_STATUS, &linkStatus));

    if (linkStatus != GL_TRUE) {
      int max_len;
      int len;
      GL_CHECK(glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &max_len));
      std::string message(max_len + 1, ' ');
      GL_CHECK(glGetProgramInfoLog(_id, max_len, &len, &message[0]));
      if (len > 0) {
        std::cout << "Failed to link _program: " << message << std::endl;
        for (auto& s : _sources) {
          std::cout << s._key << std::endl;
        }
      }
      std::cout << "Failed to link _program but error log is empty" << std::endl;
      for (auto& s : _sources) {
        std::cout << s._key << std::endl;
      }
    }
    for (const auto& s : _sources) {
      GL_CHECK(glDetachShader(_id, s._id));
      GL_CHECK(glDeleteShader(s._id));
    }
  }
  void GLShaderProgram::bind() const
  {
    GL_CHECK(glUseProgram(_id));
  }
  const std::vector<GLShaderSource>& GLShaderProgram::getSources() const
  {
    return _sources;
  }
}