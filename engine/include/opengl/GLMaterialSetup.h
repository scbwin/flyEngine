#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

#include <opengl/OpenGLAPI.h>

namespace fly
{
  class GLShaderProgram;

  class GLMaterialSetup
  {
  public:
    GLMaterialSetup() = delete;
    static void setupDiffuse(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
    static void setupAlpha(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
    static void setupNormal(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
    static void setupHeight(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
    static void setupDiffuseColor(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
    static void setupRelief(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
}

#endif
