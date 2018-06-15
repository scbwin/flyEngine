#ifndef MCWIDGET_H
#define MCWIDGET_H

#include <BaseQtScene.h>
#include <vector>
#include <Vertex.h>
#include <functional>

namespace fly
{
  class Cube;
}

class MCWidget : public BaseQtScene
{
public:
  MCWidget();
  virtual ~MCWidget();
protected:
  virtual void initScene() override;
private:
    bool createPolys(const fly::Cube& cube, float thresh, std::function<float(const fly::Vec3f&)> density_func,
      std::vector<fly::Vertex>& vertices, std::vector<unsigned>& indices);
    fly::Vec3f interpolate(float thresh, const fly::Vec3f& p0, const fly::Vec3f& p1, const std::function<float(const fly::Vec3f&)>& density_func);
    
};

#endif // !MCWIDGET_H
