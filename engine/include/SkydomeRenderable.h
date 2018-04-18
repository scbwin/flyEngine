#ifndef SKYDOMERENDERABLE_H
#define SKYDOMERENDERABLE_H

#include <Component.h>
#include <memory>

namespace fly
{
  class Mesh;

  class SkydomeRenderable : public Component
  {
  public:
    SkydomeRenderable(const std::shared_ptr<Mesh>& mesh);
    const std::shared_ptr<Mesh>& getMesh() const;
  private:
    std::shared_ptr<Mesh> _mesh;
  };
}

#endif
