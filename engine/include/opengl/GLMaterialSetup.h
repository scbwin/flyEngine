#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

namespace fly
{
  class GLShaderProgram;
  class OpenGLAPI;
  template<typename API>
  class MaterialDesc;

  class GLMaterialSetup
  {
  public:
    GLMaterialSetup() = delete;
    static void setupDiffuse(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupAlpha(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupNormal(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupHeight(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupDiffuseColor(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupRelief(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupMaterialConstants(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupDiffuseColors(GLShaderProgram const * shader, const MaterialDesc<OpenGLAPI>& desc);
  };
}

#endif
