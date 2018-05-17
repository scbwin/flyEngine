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
    static void setupDiffuse(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupAlpha(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupNormal(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupHeight(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupDiffuseColor(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupRelief(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
    static void setupMaterialConstants(GLShaderProgram* shader, const MaterialDesc<OpenGLAPI>& desc);
  };
}

#endif
