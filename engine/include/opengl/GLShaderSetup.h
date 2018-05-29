#ifndef GLSHADERSETUP_H
#define GLSHADERSETUP_H

namespace fly
{
  struct GlobalShaderParams;
  class GLShaderProgram;

  class GLShaderSetup
  {
  public:
    GLShaderSetup() = delete;
    static void setupVP(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupWorldToLight(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupLighting(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupShadows(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupTime(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupWind(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupGamma(const GlobalShaderParams& params, GLShaderProgram const * shader);
    static void setupVInverse(const GlobalShaderParams& params, GLShaderProgram const * shader);
  };
}

#endif
