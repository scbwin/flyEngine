#include <opengl/GLShaderSetup.h>
#include <opengl/GLSLShaderGenerator.h>
#include <opengl/GLShaderInterface.h>
#include <opengl/GLShaderProgram.h>
#include <opengl/OpenGLAPI.h>
#include <GlobalShaderParams.h>

namespace fly
{
  void GLShaderSetup::setupVP(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setMatrix(shader->uniformLocation(GLSLShaderGenerator::viewProjectionMatrix()), *params._VP);
  }
  void GLShaderSetup::setupWorldToLight(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setMatrixArray(shader->uniformLocation(GLSLShaderGenerator::worldToLightMatrices()), *params._worldToLight.begin(), static_cast<unsigned>(params._worldToLight.size()));
  }
  void GLShaderSetup::setupLighting(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::lightPositionWorld()), *params._lightPosWorld);
    setVector(shader->uniformLocation(GLSLShaderGenerator::lightIntensity()), *params._lightIntensity);
    setVector(shader->uniformLocation(GLSLShaderGenerator::cameraPositionWorld()), params._camPosworld);
  }
  void GLShaderSetup::setupShadows(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::shadowSampler()), OpenGLAPI::miscTexUnit0());
    setupWorldToLight(params, shader);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::numfrustumSplits()), static_cast<int>(params._smFrustumSplits->size()));
    setScalarArray(shader->uniformLocation(GLSLShaderGenerator::frustumSplits()), params._smFrustumSplits->front(), static_cast<unsigned>(params._smFrustumSplits->size()));
    setScalar(shader->uniformLocation(GLSLShaderGenerator::shadowDarkenFactor()), params._shadowDarkenFactor);
    setVector(shader->uniformLocation(GLSLShaderGenerator::viewMatrixThirdRow()), params._viewMatrix.row(2));
  }
  void GLShaderSetup::setupTime(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::time()), params._time);
  }
  void GLShaderSetup::setupWind(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setVector(shader->uniformLocation(GLSLShaderGenerator::windDir()), params._windParams._dir);
    setVector(shader->uniformLocation(GLSLShaderGenerator::windMovement()), params._windParams._movement);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::windFrequency()), params._windParams._frequency);
    setScalar(shader->uniformLocation(GLSLShaderGenerator::windStrength()), params._windParams._strength);
  }
  void GLShaderSetup::setupGamma(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setScalar(shader->uniformLocation(GLSLShaderGenerator::gamma()), params._gamma);
  }
  void GLShaderSetup::setupVInverse(const GlobalShaderParams & params, GLShaderProgram const * shader)
  {
    setMatrixTranspose(shader->uniformLocation(GLSLShaderGenerator::viewInverse()), params._viewMatrixInverse);
  }
}