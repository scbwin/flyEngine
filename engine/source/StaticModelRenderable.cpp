#include <StaticModelRenderable.h>
#include <Transform.h>
#include <Model.h>

namespace fly
{
  StaticModelRenderable::StaticModelRenderable(const std::vector<std::shared_ptr<Model>>& lods, const std::shared_ptr<Transform>& transform, float lod_divisor) :
    _lods(lods),
    _modelMatrix(transform->getModelMatrix()),
    _maxLod(lods.size() - 1),
    _lodMultiplier(1.f / lod_divisor)
  {
    auto bb_min = lods[0]->getAABB()->getMin();
    auto bb_max = lods[0]->getAABB()->getMax();
    for (unsigned i = 1; i < lods.size(); i++) {
      bb_min = minimum(bb_min, lods[i]->getAABB()->getMin());
      bb_max = maximum(bb_max, lods[i]->getAABB()->getMax());
    }
    _aabbWorld = std::make_shared<AABB>(AABB(bb_min, bb_max), _modelMatrix);
  }
  const std::shared_ptr<AABB>& StaticModelRenderable::getAABBWorld() const
  {
    return _aabbWorld;
  }
  const Mat4f & StaticModelRenderable::getModelMatrix() const
  {
    return _modelMatrix;
  }
  const std::vector<std::shared_ptr<Model>>& StaticModelRenderable::getLods() const
  {
    return _lods;
  }
  unsigned StaticModelRenderable::selectLod(const Vec3f& cam_pos) const
  {
    float dist_to_cam = (cam_pos - Vec3f({ _modelMatrix[3][0], _modelMatrix[3][1], _modelMatrix[3][2] })).length();
    return std::min(_maxLod, static_cast<unsigned>(dist_to_cam * _lodMultiplier));
  }
}