#ifndef GLMATERIALSETUP_H
#define GLMATERIALSETUP_H

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
  class GLShaderProgram;
  class OpenGLAPI;
  template<typename API, typename BV>
  class MaterialDesc;

  template<typename API, typename BV>
  class GLMaterialSetup
  {
  public:
    GLMaterialSetup() = delete;
    static void GLMaterialSetup::setupDiffuse(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::diffuseTexUnit()));
      desc.getTexture(Material::TextureKey::ALBEDO)->bind();
      setScalar(shader.uniformLocation(GLSLShaderGenerator::diffuseSampler()), OpenGLAPI::diffuseTexUnit());
    }
    static void GLMaterialSetup::setupAlpha(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::alphaTexUnit()));
      desc.getTexture(Material::TextureKey::ALPHA)->bind();
      setScalar(shader.uniformLocation(GLSLShaderGenerator::alphaSampler()), OpenGLAPI::alphaTexUnit());
    }
    static void GLMaterialSetup::setupNormal(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::normalTexUnit()));
      desc.getTexture(Material::TextureKey::NORMAL)->bind();
      setScalar(shader.uniformLocation(GLSLShaderGenerator::normalSampler()), OpenGLAPI::normalTexUnit());
    }
    static void GLMaterialSetup::setupHeight(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      GL_CHECK(glActiveTexture(GL_TEXTURE0 + OpenGLAPI::heightTexUnit()));
      desc.getTexture(Material::TextureKey::HEIGHT)->bind();
      setScalar(shader.uniformLocation(GLSLShaderGenerator::heightSampler()), OpenGLAPI::heightTexUnit());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxHeightScale()), desc.getMaterial()->getParallaxHeightScale());
    }
    static void GLMaterialSetup::setupDiffuseColor(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      setVector(shader.uniformLocation(GLSLShaderGenerator::diffuseColor()), desc.getMaterial()->getDiffuseColor());
    }
    static void GLMaterialSetup::setupRelief(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxMinSteps()), desc.getMaterial()->getParallaxMinSteps());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxMaxSteps()), desc.getMaterial()->getParallaxMaxSteps());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::parallaxBinarySearchSteps()), desc.getMaterial()->getParallaxBinarySearchSteps());
    }
    static void GLMaterialSetup::setupMaterialConstants(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      setScalar(shader.uniformLocation(GLSLShaderGenerator::ambientConstant()), desc.getMaterial()->getKa());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::diffuseConstant()), desc.getMaterial()->getKd());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::specularConstant()), desc.getMaterial()->getKs());
      setScalar(shader.uniformLocation(GLSLShaderGenerator::specularExponent()), desc.getMaterial()->getSpecularExponent());
    }
    static  void GLMaterialSetup::setupDiffuseColors(GLShaderProgram const & shader, const MaterialDesc<OpenGLAPI, BV>& desc)
    {
      desc.getDiffuseColorBuffer().bindBase(GLSLShaderGenerator::bufferBindingDiffuseColors());
    }
  };
}

#endif
