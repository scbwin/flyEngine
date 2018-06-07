#ifndef GEOMETRYGENERATOR_H
#define GEOMETRYGENERATOR_H

#include <math/FlyMath.h>
#include <vector>
#include <map>
#include <array>

namespace fly
{
  class GeometryGenerator
  {
  public:
    GeometryGenerator() = default;
    virtual ~GeometryGenerator() = default;
    enum SkirtFlag : unsigned
    {
      North = 1u,
      East = 2u,
      South = 4u,
      West = 8u
    };
    void generateGrid(int size, std::vector<glm::vec2>& vertices, std::vector<unsigned>& indices);
    std::vector<unsigned> getCubeLineIndices() const;
    struct IndexBufferInfo
    {
      unsigned _numIndices;
      unsigned _offset;
    };
    void generateGeoMipMap(unsigned size, unsigned tile_size, unsigned num_lods, std::vector<glm::vec2>& vertices, std::vector<unsigned>& indices, 
      std::map<unsigned, std::map<unsigned, IndexBufferInfo>>& index_offsets);
  private:
    std::vector<unsigned> genGMMIndices(unsigned size, unsigned lod, unsigned flag);
  };
}

#endif // !GEOMETRYGENERATOR_H
