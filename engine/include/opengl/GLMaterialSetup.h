#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

#include <opengl/OpenGLAPI.h>

namespace fly
{
  class GLMaterialSetup
  {
  public:
    GLMaterialSetup();
    virtual ~GLMaterialSetup() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) = 0;
  private:
  };

  class SetupDiffuseColor : public GLMaterialSetup
  {
  public:
    virtual ~SetupDiffuseColor() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };

  class SetupDiffuseMap : public GLMaterialSetup
  {
  public:
    virtual ~SetupDiffuseMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };

  class SetupAlphaMap : public SetupDiffuseMap
  {
  public:
    virtual ~SetupAlphaMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
}

#endif