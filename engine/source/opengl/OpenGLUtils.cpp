#include "opengl/OpenGLUtils.h"

void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
  GLenum err = glGetError();
  std::string err_string = "";
  if (err != GL_NO_ERROR)
  {
    if (err == GL_INVALID_ENUM) {
      err_string = "GL_INVALID_ENUM";
    }
    else if (err == GL_INVALID_VALUE) {
      err_string = "GL_INVALID_VALUE";
    }
    else if (err == GL_INVALID_OPERATION) {
      err_string = "GL_INVALID_OPERATION";
    }
    else if (err == GL_INVALID_FRAMEBUFFER_OPERATION) {
      err_string = "GL_INVALID_FRAMEBUFFER_OPERATION";
    }
    else if (err == GL_OUT_OF_MEMORY) {
      err_string = "GL_OUT_OF_MEMORY";
    }
    else if (err == GL_STACK_UNDERFLOW) {
      err_string = "GL_STACK_UNDERFLOW";
    }
    else if (err == GL_STACK_OVERFLOW) {
      err_string = "GL_STACK_OVERFLOW";
    }

    printf("OpenGL error %s, at %s:%i - for %s\n", err_string.c_str(), fname, line, stmt);
    abort();
  }
}