#include <opengl/GLMaterialSetup.h>
#include <GL/glew.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLUtils.h>
#include <opengl/OpenGLAPI.h>
#include <opengl/GLSLShaderGenerator.h>
#include <Material.h>
#include <MaterialDesc.h>

namespace fly
{
  void GLMaterialSetup::setupDiffuse(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::diffuseTexUnit()));
    desc.diffuseMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::diffuseSampler()), OpenGLAPI::diffuseTexUnit());
  }
  void GLMaterialSetup::setupAlpha(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::alphaTexUnit()));
    desc.alphaMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::alphaSampler()), OpenGLAPI::alphaTexUnit());
  }
  void GLMaterialSetup::setupNormal(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::normalTexUnit()));
    desc.normalMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::normalSampler()), OpenGLAPI::normalTexUnit());
  }
  void GLMaterialSetup::setupHeight(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::heightTexUnit()));
    desc.heightMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::heightSampler()), OpenGLAPI::heightTexUnit());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxHeightScale()), desc.getMaterial()->getParallaxHeightScale());
  }
  void GLMaterialSetup::setupDiffuseColor(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::diffuseColor()), desc.getMaterial()->getDiffuseColor());
  }
  void GLMaterialSetup::setupRelief(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMinSteps()), desc.getMaterial()->getParallaxMinSteps());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMaxSteps()), desc.getMaterial()->getParallaxMaxSteps());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxBinarySearchSteps()), desc.getMaterial()->getParallaxBinarySearchSteps());
  }
  void GLMaterialSetup::setupMaterialConstants(GLShaderProgram * shader, const MaterialDesc<OpenGLAPI>& desc)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::ambientConstant()), desc.getMaterial()->getKa());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::diffuseConstant()), desc.getMaterial()->getKd());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::specularConstant()), desc.getMaterial()->getKs());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::specularExponent()), desc.getMaterial()->getSpecularExponent());
  }
}