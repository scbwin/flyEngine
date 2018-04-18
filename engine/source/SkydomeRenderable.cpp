#include <SkydomeRenderable.h>

namespace fly
{
  SkydomeRenderable::SkydomeRenderable(const std::shared_ptr<Mesh>& mesh) : _mesh(mesh)
  {
  }
  const std::shared_ptr<Mesh>& SkydomeRenderable::getMesh() const
  {
    return _mesh;
  }
}