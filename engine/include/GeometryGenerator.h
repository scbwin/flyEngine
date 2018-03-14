#ifndef GEOMETRYGENERATOR_H
#define GEOMETRYGENERATOR_H

#include <glm/glm.hpp>
#include <vector>
#include <map>

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
    struct IndexBufferInfo
    {
      unsigned _numIndices;
      unsigned _offset;
    };
    void generateGeoMipMap(int size, int tile_size, unsigned num_lods, std::vector<glm::vec2>& vertices, std::vector<unsigned>& indices, 
      std::map<unsigned, std::map<unsigned, IndexBufferInfo>>& index_offsets);
  private:
    std::vector<unsigned> genGMMIndices(int size, unsigned lod, unsigned flag);
  };
}

#endif // !GEOMETRYGENERATOR_H
