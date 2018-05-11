#include <opengl/GLMaterialSetup.h>
#include <GL/glew.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLUtils.h>
#include <opengl/OpenGLAPI.h>
#include <opengl/GLSLShaderGenerator.h>
#include <Material.h>

namespace fly
{
  IMaterialSetup* GLMaterialSetup::getDiffuseSetup()
  {
    return &_diffuseSetup;
  }
  IMaterialSetup* GLMaterialSetup::getNormalSetup()
  {
    return &_normalSetup;
  }
  IMaterialSetup* GLMaterialSetup::getAlphaSetup()
  {
    return &_alphaSetup;
  }
  IMaterialSetup* GLMaterialSetup::getHeightSetup()
  {
    return &_heightSetup;
  }
  IMaterialSetup* GLMaterialSetup::getDiffuseColorSetup()
  {
    return &_diffuseColorSetup;
  }
  IMaterialSetup* GLMaterialSetup::getReliefMappingSetup()
  {
    return &_reliefMappingSetup;
  }
  void SetupDiffuse::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::diffuseTexUnit()));
    desc.diffuseMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::diffuseSampler()), OpenGLAPI::diffuseTexUnit());
  }
  void SetupAlpha::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::alphaTexUnit()));
    desc.alphaMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::alphaSampler()), OpenGLAPI::alphaTexUnit());
  }
  void SetupNormal::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::normalTexUnit()));
    desc.normalMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::normalSampler()), OpenGLAPI::normalTexUnit());
  }
  void SetupHeight::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::heightTexUnit()));
    desc.heightMap()->bind();
    setScalar(shader->uniformLocation(GLSLShaderGenerator::heightSampler()), OpenGLAPI::heightTexUnit());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxHeightScale()), desc.getMaterial()->getParallaxHeightScale());
  }
  void SetupDiffuseColor::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::diffuseColor()), desc.getMaterial()->getDiffuseColor());
  }
  void SetupReliefMapping::setup(GLShaderProgram * shader, const OpenGLAPI::MaterialDesc & desc)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMinSteps()), desc.getMaterial()->getParallaxMinSteps());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxMaxSteps()), desc.getMaterial()->getParallaxMaxSteps());
    setScalar(shader->uniformLocation(GLSLShaderGenerator::parallaxBinarySearchSteps()), desc.getMaterial()->getParallaxBinarySearchSteps());
  }
}