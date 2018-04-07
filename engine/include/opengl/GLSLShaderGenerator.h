#ifndef GLSLSHADERGENERATOR_H
#define GLSLSHADERGENERATOR_H

#include <string>
#include <vector>

namespace fly
{
  class GLSLShaderGenerator
  {
  public:
    enum MeshRenderFlag : unsigned
    {
      NONE = 0,
      DIFFUSE_MAP = 1, 
      NORMAL_MAP = 2, 
      ALPHA_MAP = 4
    };
    std::string createMeshFragmentShaderFile(unsigned flags, bool shadows);
    void regenerateShaders(bool shadows);
  private:
    std::string createMeshFragmentFile(const std::string& fname, unsigned flags, bool shadows) const;
    std::vector<std::string> _fnames;
    std::vector<unsigned> _flags;
  };
}

#endif
