#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <opengl/GLMaterialSetup.h>
#include <opengl/GLWrappers.h>
#include <Material.h>
#include <opengl/GLTexture.h>

namespace fly
{
  GLMaterialSetup::GLMaterialSetup()
  {
  }
  void SetupDiffuseColor::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    const auto& col = desc.getMaterial()->getDiffuseColor();
    GL_CHECK(glUniform3f(desc.getShader()->uniformLocation("d_col"), col[0], col[1], col[2]));
  }
  void SetupDiffuseMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    desc.getDiffuseMap()->bind();
    GL_CHECK(glUniform1i(desc.getShader()->uniformLocation("ts_diff"), 0));
  }
  void SetupAlphaMap::setup(const OpenGLAPI::MaterialDesc & desc)
  {
    SetupDiffuseMap::setup(desc);
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    desc.getAlphaMap()->bind();
    GL_CHECK(glUniform1i(desc.getShader()->uniformLocation("ts_alpha"), 1));
  }
}