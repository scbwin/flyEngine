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
    enum MeshRenderFlag : unsigned
    {
      NONE = 0,
      DIFFUSE_MAP = 1, 
      NORMAL_MAP = 2, 
      ALPHA_MAP = 4,
      PARALLAX_MAP = 8
    };
    std::string createMeshFragmentShaderFile(unsigned flags, const Settings& settings);
    void regenerateShaders(const Settings& settings);
    static inline constexpr const char* diffuseSampler() { return "ts_d"; };
    static inline constexpr const char* alphaSampler() { return "ts_a"; };
    static inline constexpr const char* normalSampler() { return "ts_n"; };
    static inline constexpr const char* heightSampler() { return "ts_h"; };
    static inline constexpr const char* parallaxHeightScale() { return "pm_h"; };
    static inline constexpr const char* shadowMapBias() { return "sm_b"; };
  private:
    std::string createMeshFragmentFile(const std::string& fname, unsigned flags, const Settings& settings) const;
    std::vector<std::string> _fnames;
    std::vector<unsigned> _flags;
  };
}

#endif
