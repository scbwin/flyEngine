#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

#include <opengl/OpenGLAPI.h>

namespace fly
{
  class GLShaderProgram;

  class IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) = 0;
  };
  class SetupDiffuse : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
  class SetupAlpha : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
  class SetupNormal : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
  class SetupHeight : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
  class SetupDiffuseColor : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };
  class SetupReliefMapping : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc);
  };

  class GLMaterialSetup
  {
  public:
    IMaterialSetup* getDiffuseSetup();
    IMaterialSetup* getNormalSetup();
    IMaterialSetup* getAlphaSetup();
    IMaterialSetup* getHeightSetup();
    IMaterialSetup* getDiffuseColorSetup();
    IMaterialSetup* getReliefMappingSetup();
  private:
    SetupDiffuse _diffuseSetup;
    SetupNormal _normalSetup;
    SetupAlpha _alphaSetup;
    SetupHeight _heightSetup;
    SetupDiffuseColor _diffuseColorSetup;
    SetupReliefMapping _reliefMappingSetup;
  };
}

#endif
