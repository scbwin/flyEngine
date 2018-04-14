#include <StaticMeshRenderable.h>
#include <Mesh.h>

namespace fly
{
  StaticMeshRenderable::StaticMeshRenderable(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, 
    const Mat4f & model_matrix, bool has_wind, const Vec3f& aabb_offset) :
    _mesh(mesh),
    _material(material),
    _modelMatrix(model_matrix),
    _modelMatrixInverse(inverse(glm::mat3(model_matrix))),
    _hasWind(has_wind)
  {
    Vec3f bb_min(std::numeric_limits<float>::max());
    Vec3f bb_max(std::numeric_limits<float>::lowest());
    for (const auto& v : mesh->getVertices()) {
      Vec3f v_world = (model_matrix * Vec4f(v._position, 1.f)).xyz();
      bb_min = minimum(bb_min, v_world);
      bb_max = maximum(bb_max, v_world);
    }
    bb_min -= aabb_offset;
    bb_max += aabb_offset;
    _aabbWorld = std::make_unique<AABB>(bb_min, bb_max);
    _windParams._pivotWorld = _aabbWorld->getMax()[1];
    _windParams._bendFactorExponent = 2.f;
  }
  AABB* StaticMeshRenderable::getAABBWorld() const
  {
    return _aabbWorld.get();
  }
  const std::shared_ptr<Mesh>& StaticMeshRenderable::getMesh() const
  {
    return _mesh;
  }
  const std::shared_ptr<Material>& StaticMeshRenderable::getMaterial() const
  {
    return _material;
  }
  const Mat4f& StaticMeshRenderable::getModelMatrix() const
  {
    return _modelMatrix;
  }
  const Mat3f& StaticMeshRenderable::getModelMatrixInverse() const
  {
    return _modelMatrixInverse;
  }
  bool StaticMeshRenderable::hasWind() const
  {
    return _hasWind;
  }
  void StaticMeshRenderable::setHasWind(bool has_wind)
  {
    _hasWind = has_wind;
  }
  const WindParamsLocal & StaticMeshRenderable::getWindParams() const
  {
    return _windParams;
  }
  void StaticMeshRenderable::setWindParams(const WindParamsLocal & params)
  {
    _windParams = params;
  }
}