#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <opengl/GLMaterialSetup.h>
#include <opengl/GLWrappers.h>
#include <Material.h>
#include <opengl/GLTexture.h>
#include <opengl/GLShaderInterface.h>

namespace fly
{
  GLMaterialSetup::GLMaterialSetup()
  {
  }
  void SetupDiffuseColor::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    setVector(desc.getShader()->uniformLocation("d_col"), desc.getMaterial()->getDiffuseColor());
  }
  void SetupDiffuseMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    desc.getDiffuseMap()->bind();
    setScalar(desc.getShader()->uniformLocation("ts_diff"), 0);
  }
  void SetupAlphaMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    desc.getAlphaMap()->bind();
    setScalar(desc.getShader()->uniformLocation("ts_alpha"), 1);
  }
  void SetupNormalMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    desc.getNormalMap()->bind();
    setScalar(desc.getShader()->uniformLocation("ts_norm"), 2);
  }
  void SetupDiffuseAlphaMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    SetupDiffuseMap::setup(desc);
    SetupAlphaMap::setup(desc);
  }
  void SetupDiffuseNormalMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    SetupDiffuseMap::setup(desc);
    SetupNormalMap::setup(desc);
  }
  void SetupDiffuseAlphaNormalMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    SetupDiffuseMap::setup(desc);
    SetupAlphaMap::setup(desc);
    SetupNormalMap::setup(desc);
  }
  void SetupDiffuseColorNormalMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    SetupDiffuseColor::setup(desc);
    SetupNormalMap::setup(desc);
  }
}