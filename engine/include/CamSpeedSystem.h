#ifndef CAMSPEEDSYSTEM_H
#define CAMSPEEDSYSTEM_H

#include <System.h>
#include <renderer/Renderer.h>
#include <PhysicsCameraController.h>

namespace fly
{
  /**
  * Controls the speed of the camera, depending if the camera intersects any of the scenes static geometry.
  * This class is only used for demonstration purposes on how to perform intersection tests with a 
  * Bounding Volume Hierarchy (BVH)
  */
  template<typename API, typename BV>
  class CamSpeedSystem : public System
  {
  public:
    CamSpeedSystem(const Renderer<API, BV>& renderer, const std::shared_ptr<PhysicsCameraController>& camera_controller) :
      _bvhStatic(renderer.getStaticBVH()),
      _camController(camera_controller)
    {
    }
    virtual ~CamSpeedSystem() = default;
    virtual void update() override
    {
      AABB aabb(_camController->getCamera()->getPosition() - _range, _camController->getCamera()->getPosition() + _range);
      _intersectedObjects.clear();
      _bvhStatic->intersectObjects(aabb, _intersectedObjects);
      _camController->setDamping(_intersectedObjects.size() ? 0.7f : PhysicsCameraController::DEFAULT_DAMPING);
    }
  private:
    std::unique_ptr<typename Renderer<API>::BVH> const & _bvhStatic;
    StackPOD<IMeshRenderable<API, BV>*> _intersectedObjects;
    std::shared_ptr<PhysicsCameraController> const & _camController;
    float _range = 1.f;
  };
}

#endif // !CAMSPEEDSYSTEM_H
