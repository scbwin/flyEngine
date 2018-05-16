#include <opengl/GLShaderSetup.h>
#include <opengl/GLSLShaderGenerator.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLAPI.h>

namespace fly
{
  void GLShaderSetup::setupVP(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setMatrix(shader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix()), *params._VP);
  }
  void GLShaderSetup::setupLighting(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::lightPositionWorld()), *params._lightPosWorld);
    setVector(shader->uniformLocation(GLSLShaderGenerator::lightIntensity()), *params._lightIntensity);
    setVector(shader->uniformLocation(GLSLShaderGenerator::cameraPositionWorld()), params._camPosworld);
  }
  void GLShaderSetup::setupShadows(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::shadowSampler()), OpenGLAPI::miscTexUnit0());
    setMatrixArray(shader->uniformLocation(GLSLShaderGenerator::worldToLightMatrices()), *params._worldToLight.begin(), static_cast<unsigned>(params._worldToLight.size()));
    setScalar(shader->uniformLocation(GLSLShaderGenerator::numfrustumSplits()), static_cast<int>(params._worldToLight.size()));
    setScalarArray(shader->uniformLocation(GLSLShaderGenerator::frustumSplits()), params._smFrustumSplits->front(), static_cast<unsigned>(params._smFrustumSplits->size()));
    setScalar(shader->uniformLocation(GLSLShaderGenerator::shadowMapBias()), params._smBias);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::shadowDarkenFactor()), params._shadowDarkenFactor);
  }
  void GLShaderSetup::setupTime(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::time()), params._time);
  }
  void GLShaderSetup::setupWind(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::windDir()), params._windParams._dir);
    setVector(shader->uniformLocation(GLSLShaderGenerator::windMovement()), params._windParams._movement);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::windFrequency()), params._windParams._frequency);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::windStrength()), params._windParams._strength);
  }
  void GLShaderSetup::setupGamma(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::gamma()), params._gamma);
  }
  void GLShaderSetup::setupPInverse(const GlobalShaderParams & params, GLShaderProgram* shader)
  {
    setMatrix(shader->uniformLocation(GLSLShaderGenerator::projectionMatrixInverse()), inverse(params._projectionMatrix));
  }
}