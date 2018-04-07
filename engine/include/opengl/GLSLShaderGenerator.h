#ifndef GLSLSHADERGENERATOR_H
#define GLSLSHADERGENERATOR_H

#include <string>

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
  private:
    std::string createMeshFragmentFile(const std::string& fname, unsigned flags, bool shadows);
  };
}

#endif
