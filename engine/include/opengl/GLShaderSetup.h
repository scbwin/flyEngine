#ifndef GLSHADERSETUP_H
#define GLSHADERSETUP_H

#include <opengl/OpenGLAPI.h>

namespace fly
{
  struct GlobalShaderParams;

  class GLShaderSetup
  {
  public:
    GLShaderSetup() = delete;
    static void setupVP(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupLighting(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupShadows(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupTime(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupWind(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupGamma(const GlobalShaderParams& params, GLShaderProgram* shader);
    static void setupPInverse(const GlobalShaderParams& params, GLShaderProgram* shader);
  };
}

#endif
