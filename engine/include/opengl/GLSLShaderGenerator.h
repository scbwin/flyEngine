#ifndef GLSLSHADERGENERATOR_H
#define GLSLSHADERGENERATOR_H

#include <string>
#include <vector>
#include <opengl/GLShaderSource.h>

namespace fly
{
  struct Settings;
  class GraphicsSettings;

  class GLSLShaderGenerator
  {
  public:
    GLSLShaderGenerator();
    GLShaderSource createMeshVertexShaderSource(unsigned flags, const GraphicsSettings& settings, bool instanced = false) const;
    GLShaderSource createMeshFragmentShaderSource(unsigned flags, const GraphicsSettings& settings, bool instanced = false) const;
    GLShaderSource createMeshVertexShaderDepthSource(unsigned flags, const GraphicsSettings& settings, bool instanced = false) const;
    GLShaderSource createMeshGeometryShaderDepthSource(unsigned flags, const GraphicsSettings& settings, bool instanced = false) const;
    GLShaderSource createMeshFragmentShaderDepthSource(unsigned flags, const GraphicsSettings& settings) const;
    void createCompositeShaderSource(const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src) const;
    void createBlurShaderSource(const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src) const;
    void createSSRShaderSource(const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src)const;
    void createGodRayShaderSource(const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src) const;
    static constexpr const char* diffuseSampler = "ts_d";
    static constexpr const char* alphaSampler = "ts_a";
    static constexpr const char* normalSampler = "ts_n";
    static constexpr const char* heightSampler = "ts_h";
    static constexpr const char* shadowSampler = "ts_sm";
    static constexpr const char* lightingSampler = "ts_l";
    static constexpr const char* occlusionSampler  ="ts_o";
    static constexpr const char* godRaySampler = "ts_g";
    static constexpr const char* lightPosUV = "lp_uv";
    static constexpr const char* parallaxHeightScale = "pm_h";
    static constexpr const char* parallaxMinSteps = "p_min";
    static constexpr const char* parallaxMaxSteps = "p_max";
    static constexpr const char* parallaxBinarySearchSteps = "pbss";
    static constexpr const char* shadowMapBias = "sm_b";
    static constexpr const char* windDir = "wd";
    static constexpr const char* windMovement = "wm";
    static constexpr const char* windFrequency = "wf";
    static constexpr const char* windStrength = "ws";
    static constexpr const char* windPivot = "wp";
    static constexpr const char* windExponent = "we";
    static constexpr const char* bbMin = "bb_mi";
    static constexpr const char* bbMax = "bb_ma";
    static constexpr const char* time = "t";
    static constexpr const char* modelMatrix = "M";
    static constexpr const char* modelMatrixInverse = "M_i";
    static constexpr const char* viewInverse = "V_i";
    static constexpr const char* viewProjectionMatrix = "VP";
    static constexpr const char* projectionMatrix = "P";
    static constexpr const char* projectionMatrixInverse = "P_i";
    static constexpr const char* projectionMatrixInverseThirdRow = "P_i_3";
    static constexpr const char* projectionMatrixInverseFourthRow = "P_i_4";
    static constexpr const char* modelViewProjectionMatrix = "MVP";
    static constexpr const char* lightDirWorld = "ld_w";
    static constexpr const char* cameraPositionWorld = "cp_ws";
    static constexpr const char* lightIntensity = "I_in";
    static constexpr const char* godRayIntensity = "gi";
    static constexpr const char* worldToLightMatrices = "wtl";
    static constexpr const char* frustumSplits = "fs";
    static constexpr const char* numfrustumSplits = "nfs";
    static constexpr const char* shadowDarkenFactor = "sdf";
    static constexpr const char* diffuseColor = "dcol";
    static constexpr const char* gamma = "g";
    static constexpr const char* ambientConstant = "ka";
    static constexpr const char* diffuseConstant = "kd";
    static constexpr const char* specularConstant = "ks";
    static constexpr const char* specularExponent = "se";
    static constexpr const char* texelSize = "ts";
    static constexpr const char* toBlurSampler = "ts_b";
    static constexpr const char* depthSampler = "ts_d";
    static constexpr const char* dofSampler = "ts_dof";
    static constexpr const char* viewSpaceNormalsSampler = "ts_n";
    static constexpr const char* viewMatrixThirdRow = "v3";
    static constexpr const unsigned bufferBindingAABB = 0;
    static constexpr const unsigned bufferBindingVisibleInstances = 1;
    static constexpr const unsigned bufferBindingIndirectInfo = 2;
    static constexpr const unsigned bufferBindingInstanceData = 3;
    static constexpr const unsigned bufferBindingDiffuseColors = 4;
    static constexpr const char* noiseCodeGLSL() {
      return "float hash(vec2 p)\n\
{\n\
  return fract(sin(dot(p, vec2(24.33f, 52.23f))) * 14721.28f);\n\
}\n\
float noise(vec2 p)\n\
{\n\
  vec2 start = floor(p);\n\
  vec2 end = start + 1.f;\n\
  vec2 weights = smoothstep(start, end, p);\n\
  return mix(mix(hash(start), hash(vec2(end.x, start.y)), weights.x), mix(hash(vec2(start.x, end.y)), hash(end), weights.x), weights.y);\n\
}\n";
    }
    static constexpr const char* vertexFileComposite = "assets/opengl/vs_screen.glsl";
    static constexpr const char* directory = "generated/";
  private:
    std::string createMeshVertexSource(unsigned flags, const GraphicsSettings& settings, bool instanced) const;
    std::string createMeshVertexDepthSource(unsigned flags, const GraphicsSettings& settings, bool instanced) const;
    std::string createMeshGeometryDepthSource(unsigned flags, const GraphicsSettings& settings, bool instanced) const;
    std::string createMeshFragmentSource(unsigned flags, const GraphicsSettings& settings, bool instanced) const;
    std::string createMeshFragmentDepthSource(unsigned flags, const GraphicsSettings& settings) const;
    std::string createCompositeShaderSource(const GraphicsSettings& gs) const;
    std::string _windParamString;
    std::string _windCodeString;
    std::string _instanceDataStr;
    GLShaderSource _compositeVertexSource;
  };
}

#endif
