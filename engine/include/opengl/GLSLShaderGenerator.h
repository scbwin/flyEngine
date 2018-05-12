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
    enum MeshRenderFlag : unsigned
    {
      NONE = 0,
      DIFFUSE_MAP = 1, 
      NORMAL_MAP = 2, 
      ALPHA_MAP = 4,
      HEIGHT_MAP = 8,
      WIND = 16
    };
    GLShaderSource createMeshVertexShaderSource(unsigned flags, const GraphicsSettings& settings);
    GLShaderSource createMeshFragmentShaderSource(unsigned flags, const GraphicsSettings& settings);
    GLShaderSource createMeshVertexShaderDepthSource(unsigned flags, const GraphicsSettings& settings);
    GLShaderSource createMeshFragmentShaderDepthSource(unsigned flags, const GraphicsSettings& settings);
    void createCompositeShaderSource(const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src);
    void createBlurShaderSource(unsigned flags, const GraphicsSettings& gs, GLShaderSource& vertex_src, GLShaderSource& fragment_src);
    static inline constexpr const char* diffuseSampler() { return "ts_d"; };
    static inline constexpr const char* alphaSampler() { return "ts_a"; };
    static inline constexpr const char* normalSampler() { return "ts_n"; };
    static inline constexpr const char* heightSampler() { return "ts_h"; };
    static inline constexpr const char* shadowSampler() { return "ts_sm"; };
    static inline constexpr const char* lightingSampler() { return "ts_l"; };
    static inline constexpr const char* parallaxHeightScale() { return "pm_h"; };
    static inline constexpr const char* parallaxMinSteps() { return "p_min"; };
    static inline constexpr const char* parallaxMaxSteps() { return "p_max"; };
    static inline constexpr const char* parallaxBinarySearchSteps() { return "pbss"; };
    static inline constexpr const char* shadowMapBias() { return "sm_b"; };
    static inline constexpr const char* windDir() { return "wd"; };
    static inline constexpr const char* windMovement() { return "wm"; };
    static inline constexpr const char* windFrequency() { return "wf"; };
    static inline constexpr const char* windStrength() { return "ws"; };
    static inline constexpr const char* windPivot() { return "wp"; };
    static inline constexpr const char* windExponent() { return "we"; };
    static inline constexpr const char* bbMin() { return "bb_mi"; };
    static inline constexpr const char* bbMax() { return "bb_ma"; };
    static inline constexpr const char* time() { return "t"; };
    static inline constexpr const char* modelMatrix() { return "M"; };
    static inline constexpr const char* modelMatrixInverse() { return "M_i"; };
    static inline constexpr const char* viewProjectionMatrix() { return "VP"; };
    static inline constexpr const char* projectionMatrixInverse() { return "P_i"; };
    static inline constexpr const char* modelViewProjectionMatrix() { return "MVP"; };
    static inline constexpr const char* lightPositionWorld() { return "lp_ws"; };
    static inline constexpr const char* cameraPositionWorld() { return "cp_ws"; };
    static inline constexpr const char* lightIntensity() { return "I_in"; };
    static inline constexpr const char* worldToLightMatrices() { return "wtl"; };
    static inline constexpr const char* frustumSplits() { return "fs"; };
    static inline constexpr const char* numfrustumSplits() { return "nfs"; };
    static inline constexpr const char* shadowDarkenFactor() { return "sdf"; };
    static inline constexpr const char* diffuseColor() { return "dcol"; };
    static inline constexpr const char* gamma() { return "g"; };
    static inline constexpr const char* ambientConstant() { return "ka"; };
    static inline constexpr const char* diffuseConstant() { return "kd"; };
    static inline constexpr const char* specularConstant() { return "ks"; };
    static inline constexpr const char* specularExponent() { return "se"; };
    static inline constexpr const char* texelSize() { return "ts"; };
    static inline constexpr const char* toBlurSampler() { return "ts_b"; };
    static inline constexpr const char* depthSampler() { return "ts_d"; };
    static inline constexpr const char* dofSampler() { return "ts_dof"; };
    static inline constexpr const char* noiseCodeGLSL() {
      return "float hash(vec2 p)\n\
{\n\
  return fract(sin(dot(p, vec2(12.9898f, 78.233f))) * 43758.5453f);\n\
}\n\
float noise(vec2 p)\n\
{\n\
  vec2 start = floor(p);\n\
  vec2 end = start + 1.f;\n\
  vec2 weights = smoothstep(start, end, p);\n\
  return mix(mix(hash(start), hash(vec2(end.x, start.y)), weights.x), mix(hash(vec2(start.x, end.y)), hash(end), weights.x), weights.y);\n\
}\n";
    }
    static inline constexpr const char* vertexFileComposite() { return "assets/opengl/vs_screen.glsl"; };
    static inline constexpr const char* directory() { return "generated/"; };
  private:
    std::string createMeshVertexSource(unsigned flags, const GraphicsSettings& settings) const;
    std::string createMeshVertexDepthSource(unsigned flags, const GraphicsSettings& settings) const;
    std::string createMeshFragmentSource(unsigned flags, const GraphicsSettings& settings) const;
    std::string createMeshFragmentDepthSource(unsigned flags, const GraphicsSettings& settings) const;
    std::string createCompositeShaderSource(const GraphicsSettings& gs) const;
    std::string _windParamString;
    std::string _windCodeString;
    GLShaderSource _compositeVertexSource;
  };
}

#endif
