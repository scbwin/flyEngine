#ifndef MESH_H
#define MESH_H

#include <vector>
#include <array>
#include <memory>
#include <functional>
#include "Vertex.h"
#include <AABB.h>
#include <Sphere.h>

namespace fly
{
  class Material;

  class Mesh
  {
  public:
    Mesh();
    Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, unsigned int material_index);

    const std::vector<Vertex>& getVertices() const;
    const std::vector<unsigned int>& getIndices() const;
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<unsigned>& indices);
    unsigned int getMaterialIndex() const;
    const AABB& getAABB() const;
    const Sphere& getSphere() const;
    void setMaterialIndex(unsigned material_index);
    void setMaterial(const std::shared_ptr<Material>& material);
    const std::shared_ptr<Material>& getMaterial() const;

  private:
    std::vector<Vertex> _vertices;
    std::vector<unsigned int> _indices;
    unsigned int _materialIndex;
    std::shared_ptr<Material> _material;
    AABB _aabb;
    Sphere _sphere;
  };
}

#endif
