#include <opengl/GLShaderSource.h>
#include <fstream>

namespace fly
{
  void GLShaderSource::initFromFile(const std::string & file, GLenum type)
  {
    _key = file;
    _type = type;
    _source.clear();
    std::ifstream is(file);
    std::string line;
    while (std::getline(is, line)) {
      _source += line + "\n";
    }
  }
}
