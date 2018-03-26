#define GLM_ENABLE_EXPERIMENTAL
#include "Mesh.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#include <AABB.h>
#include <math/FlyMath.h>


namespace fly
{
  Mesh::Mesh()
  {
  }
  Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, unsigned int material_index) :
    _vertices(vertices), _indices(indices), _materialIndex(material_index)
  {
    Vec3f bb_min(std::numeric_limits<float>::max());
    Vec3f bb_max(std::numeric_limits<float>::lowest());
    for (auto& v : _vertices) {
      bb_min = minimum(bb_min, v._position);
      bb_max = maximum(bb_max, v._position);
    }
    _aabb = std::unique_ptr<AABB>(new AABB(bb_min, bb_max));
  }

  const std::vector<Vertex>& Mesh::getVertices() const
  {
    return _vertices;
  }

  const std::vector<unsigned int>& Mesh::getIndices() const
  {
    return _indices;
  }

  std::vector<unsigned> Mesh::getAABBLineIndices()
  {
    return{ 0, 1, 1, 6, 6, 3, 3, 0,   0, 2, 2, 4, 4, 1,   2, 5, 5, 7, 7, 4,   7, 6, 5, 3 };
  }
  unsigned int Mesh::getMaterialIndex() const
  {
    return _materialIndex;
  }
  AABB* Mesh::getAABB() const
  {
    return _aabb.get();
  }
  void Mesh::setMaterialIndex(unsigned material_index)
  {
    _materialIndex = material_index;
  }
}