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
  class SetupDiffuseColor : public virtual GLMaterialSetup
  {
  public:
    virtual ~SetupDiffuseColor() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupDiffuseMap : public virtual GLMaterialSetup
  {
  public:
    virtual ~SetupDiffuseMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupAlphaMap : public virtual GLMaterialSetup
  {
  public:
    virtual ~SetupAlphaMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupNormalMap : public virtual GLMaterialSetup
  {
  public:
    virtual ~SetupNormalMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupDiffuseAlphaMap : public SetupDiffuseMap, public SetupAlphaMap
  {
  public:
    virtual ~SetupDiffuseAlphaMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupDiffuseNormalMap : public SetupDiffuseMap, public SetupNormalMap
  {
  public:
    virtual ~SetupDiffuseNormalMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
  class SetupDiffuseAlphaNormalMap : public SetupDiffuseMap, public SetupAlphaMap, public SetupNormalMap
  {
  public:
    virtual ~SetupDiffuseAlphaNormalMap() = default;
    virtual void setup(const OpenGLAPI::MaterialDesc& desc) override;
  private:
  };
}

#endif