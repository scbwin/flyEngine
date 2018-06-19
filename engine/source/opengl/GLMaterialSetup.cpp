#include <opengl/GLMaterialSetup.h>
#include <opengl/GLShaderInterface.h>
#include <Material.h>
#include <opengl/OpenGLAPI.h>
#include <MaterialDesc.h>

namespace fly
{
  void GLMaterialSetup::setupDiffuse(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::diffuseTexUnit()));
    desc.getTexture(Material::TextureKey::ALBEDO)->bind();
    setScalar(shader.uniformLocation(GLSLShaderGenerator::diffuseSampler()), OpenGLAPI::diffuseTexUnit());
  }
  void GLMaterialSetup::setupAlpha(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::alphaTexUnit()));
    desc.getTexture(Material::TextureKey::ALPHA)->bind();
    setScalar(shader.uniformLocation(GLSLShaderGenerator::alphaSampler()), OpenGLAPI::alphaTexUnit());
  }
  void GLMaterialSetup::setupNormal(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::normalTexUnit()));
    desc.getTexture(Material::TextureKey::NORMAL)->bind();
    setScalar(shader.uniformLocation(GLSLShaderGenerator::normalSampler()), OpenGLAPI::normalTexUnit());
  }
  void GLMaterialSetup::setupHeight(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::heightTexUnit()));
    desc.getTexture(Material::TextureKey::HEIGHT)->bind();
    setScalar(shader.uniformLocation(GLSLShaderGenerator::heightSampler()), OpenGLAPI::heightTexUnit());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxHeightScale()), desc.getMaterial()->getParallaxHeightScale());
  }
  void GLMaterialSetup::setupDiffuseColor(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setVector(shader.uniformLocation(GLSLShaderGenerator::diffuseColor()), desc.getMaterial()->getDiffuseColor());
  }
  void GLMaterialSetup::setupRelief(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxMinSteps()), desc.getMaterial()->getParallaxMinSteps());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxMaxSteps()), desc.getMaterial()->getParallaxMaxSteps());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxBinarySearchSteps()), desc.getMaterial()->getParallaxBinarySearchSteps());
  }
  void GLMaterialSetup::setupMaterialConstants(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setScalar(shader.uniformLocation(GLSLShaderGenerator::ambientConstant()), desc.getMaterial()->getKa());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::diffuseConstant()), desc.getMaterial()->getKd());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::specularConstant()), desc.getMaterial()->getKs());
    setScalar(shader.uniformLocation(GLSLShaderGenerator::specularExponent()), desc.getMaterial()->getSpecularExponent());
  }
  void GLMaterialSetup::setupDiffuseColors(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    desc.getDiffuseColorBuffer().bindBase(GLSLShaderGenerator::bufferBindingDiffuseColors());
  }
}
