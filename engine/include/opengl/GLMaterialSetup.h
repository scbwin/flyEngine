#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

#include <opengl/OpenGLAPI.h>

namespace fly
{
  class GLShaderProgram;

  class IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const = 0;
  };
  class SetupDiffuse : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };
  class SetupAlpha : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };
  class SetupNormal : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };
  class SetupHeight : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };
  class SetupDiffuseColor : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };
  class SetupReliefMapping : public IMaterialSetup
  {
  public:
    virtual void setup(GLShaderProgram* shader, const OpenGLAPI::MaterialDesc& desc) const;
  };

  class GLMaterialSetup
  {
  public:
    IMaterialSetup const* getDiffuseSetup();
    IMaterialSetup const* getNormalSetup();
    IMaterialSetup const* getAlphaSetup();
    IMaterialSetup const* getHeightSetup();
    IMaterialSetup const* getDiffuseColorSetup();
    IMaterialSetup const* getReliefMappingSetup();
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
