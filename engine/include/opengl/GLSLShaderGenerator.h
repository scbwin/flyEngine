#ifndef GLSLSHADERGENERATOR_H
#define GLSLSHADERGENERATOR_H

#include <string>
#include <vector>

namespace fly
{
  struct Settings;

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
      PARALLAX_MAP = 8,
      WIND = 16
    };
    std::string createMeshVertexShaderFile(unsigned flags, const Settings& settings);
    std::string createMeshFragmentShaderFile(unsigned flags, const Settings& settings);
    std::string createMeshVertexShaderFileDepth(unsigned flags, const Settings& settings);
    std::string createMeshFragmentShaderFileDepth(unsigned flags, const Settings& settings);
    void regenerateShaders(const Settings& settings);
    static inline constexpr const char* diffuseSampler() { return "ts_d"; };
    static inline constexpr const char* alphaSampler() { return "ts_a"; };
    static inline constexpr const char* normalSampler() { return "ts_n"; };
    static inline constexpr const char* heightSampler() { return "ts_h"; };
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
  private:
    std::string createMeshVertexFile(const std::string& fname, unsigned flags, const Settings& settings) const;
    std::string createMeshVertexFileDepth(const std::string& fname, unsigned flags, const Settings& settings) const;
    std::string createMeshFragmentFile(const std::string& fname, unsigned flags, const Settings& settings) const;
    std::string createMeshFragmentFileDepth(const std::string& fname, unsigned flags, const Settings& settings) const;
    std::vector<std::string> _fnamesFragment;
    std::vector<unsigned> _flagsFragment;
    std::vector<std::string> _fnamesVertex;
    std::vector<unsigned> _flagsVertex;
    std::vector<std::string> _fnamesVertexDepth;
    std::vector<unsigned> _flagsVertexDepth;
    std::vector<std::string> _fnamesFragmentDepth;
    std::vector<unsigned> _flagsFragmentDepth;
    std::string _windParamString;
    std::string _windCodeString;
  };
}

#endif
