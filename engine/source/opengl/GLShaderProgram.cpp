#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLUtils.h>
#include <iostream>
#include <regex>

namespace fly
{
  GLShaderProgram::GLShaderProgram() 
  {
    GL_CHECK(_id = glCreateProgram());
  }
  GLShaderProgram::~GLShaderProgram()
  {
    cleanup();
  }
  GLShaderProgram::GLShaderProgram(GLShaderProgram && other) :
    _id(other._id),
    _sources(std::move(other._sources)),
    _uniformLocations(std::move(other._uniformLocations)),
    _uniformNames(std::move(other._uniformNames))
  {
    other._id = 0;
  }
  GLShaderProgram & GLShaderProgram::operator=(GLShaderProgram && other)
  {
    cleanup();
    _id = other._id;
    _sources = std::move(other._sources);
    _uniformLocations = std::move(other._uniformLocations);
    _uniformNames = std::move(other._uniformNames);
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
  void GLShaderProgram::link()
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

    GLint num_uniforms;
    GL_CHECK(glGetProgramInterfaceiv(_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num_uniforms));
    int buf_size;
    GL_CHECK(glGetProgramInterfaceiv(_id, GL_UNIFORM, GL_MAX_NAME_LENGTH, &buf_size));
    _uniformNames.resize(num_uniforms);
    for (int i = 0; i < num_uniforms; i++) {
      GLsizei length;
      _uniformNames[i] = new char[buf_size];
      GL_CHECK(glGetProgramResourceName(_id, GL_UNIFORM, i, buf_size, &length, _uniformNames[i]));
      std::string str = std::regex_replace(_uniformNames[i], std::regex("\\[([0-9]+)\\]"), "");
      std::strcpy(_uniformNames[i], str.c_str());
      GL_CHECK(_uniformLocations[_uniformNames[i]] = glGetUniformLocation(_id, _uniformNames[i]));
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
  void GLShaderProgram::cleanup()
  {
    if (_id) {
      GL_CHECK(glDeleteProgram(_id));
    }
    for (const auto& n : _uniformNames) {
      delete[] n;
    }
  }
}