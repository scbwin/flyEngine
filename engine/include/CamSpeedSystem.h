#ifndef CAMSPEEDSYSTEM_H
#define CAMSPEEDSYSTEM_H

#include <System.h>
#include <renderer/Renderer.h>
#include <CameraController.h>

namespace fly
{
  /**
  * Controls the speed of the camera, depending if the camera intersects any of the scenes static geometry.
  * This class is only used for demonstration purposes on how to perform intersection tests with a 
  * Bounding Volume Hierarchy (BVH)
  */
  template<typename API>
  class CamSpeedSystem : public System
  {
  public:
    CamSpeedSystem(const Renderer<API>& renderer, const std::unique_ptr<CameraController>& camera_controller) : 
      _bvhStatic(renderer.getStaticBVH()),
      _camController(camera_controller)
    {
    }
    virtual ~CamSpeedSystem() = default;

    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) override
    {
    }
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) override
    {
    }
    virtual void update() override
    {
      AABB aabb(_camController->getCamera()->getPosition() - _range, _camController->getCamera()->getPosition() + _range);
      _intersectedObjects.clear();
      _bvhStatic->intersectObjects(aabb, _intersectedObjects);
      _camController->setSpeedFactor(_intersectedObjects.size() ? _speedFactorLow : _speedFactorHigh);
    }
  private:
    std::unique_ptr<typename Renderer<API>::BVH> const & _bvhStatic;
    StackPOD<IMeshRenderable<API>*> _intersectedObjects;
    std::unique_ptr<CameraController> const & _camController;
    float _speedFactorLow = 0.2f;
    float _speedFactorHigh = 1.f;
    float _range = 1.f;
  };
}

#endif // !CAMSPEEDSYSTEM_H
