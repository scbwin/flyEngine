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
    _vertices(vertices), 
    _indices(indices), 
    _materialIndex(material_index), 
    _aabb(*this)
  {
  }
  const std::vector<Vertex>& Mesh::getVertices() const
  {
    return _vertices;
  }

  const std::vector<unsigned int>& Mesh::getIndices() const
  {
    return _indices;
  }
  void Mesh::setVertices(const std::vector<Vertex>& vertices)
  {
    _vertices = vertices;
  }
  void Mesh::setIndices(const std::vector<unsigned>& indices)
  {
    _indices = indices;
  }
  unsigned int Mesh::getMaterialIndex() const
  {
    return _materialIndex;
  }
  AABB const * Mesh::getAABB() const
  {
    return &_aabb;
  }
  void Mesh::setMaterialIndex(unsigned material_index)
  {
    _materialIndex = material_index;
  }
  void Mesh::setMaterial(const std::shared_ptr<Material>& material)
  {
    _material = material;
  }
  const std::shared_ptr<Material>& Mesh::getMaterial() const
  {
    return _material;
  }
}