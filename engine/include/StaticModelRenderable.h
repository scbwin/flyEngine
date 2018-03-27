#ifndef STATICMODELRENDERABLE_H
#define STATICMODELRENDERABLE_H

#include <Component.h>
#include <vector>
#include <memory>
#include <math/FlyMath.h>

namespace fly
{
  class Model;
  class Transform;
  class AABB;

  class StaticModelRenderable : public Component
  {
  public:
    StaticModelRenderable(const std::vector<std::shared_ptr<Model>>& lods, const std::shared_ptr<Transform>& transform, float lod_divisor);
    virtual ~StaticModelRenderable() = default;
    const std::shared_ptr<AABB>& getAABBWorld() const;
    const Mat4f& getModelMatrix() const;
    const std::vector<std::shared_ptr<Model>>& getLods() const;
    unsigned selectLod(const Vec3f& cam_pos) const;
  private:
    std::vector<std::shared_ptr<Model>> _lods;
    Mat4f _modelMatrix;
    std::shared_ptr<AABB> _aabbWorld;
    unsigned _maxLod;
    float _lodMultiplier;
  };
}

#endif
