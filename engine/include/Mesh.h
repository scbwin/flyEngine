#ifndef MESH_H
#define MESH_H

#include <vector>
#include <array>
#include <memory>
#include <functional>
#include "Vertex.h"
#include <AABB.h>

namespace fly
{
  class Mesh
  {
  public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, unsigned int material_index);

    const std::vector<Vertex>& getVertices() const;
    const std::vector<unsigned int>& getIndices() const;
    std::vector<unsigned> getAABBLineIndices();
    unsigned int getMaterialIndex() const;
    AABB* getAABB() const;
    void setMaterialIndex(unsigned material_index);

  private:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    unsigned int _materialIndex;
    std::unique_ptr<AABB> _aabb;

  };
}

#endif // !MESH_H
