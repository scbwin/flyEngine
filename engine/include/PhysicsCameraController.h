#ifndef PHYSICSCAMERACONTROLLER_H
#define PHYSICSCAMERACONTROLLER_H

#include <memory>
#include <math/FlyMath.h>
#include <FixedTimestepSystem.h>

namespace fly
{
  class Camera;

  class PhysicsCameraController : public FixedTimestepSystem
  {
  public:
    PhysicsCameraController(const std::shared_ptr<Camera>& camera);
    virtual ~PhysicsCameraController() = default;
    virtual void saveState() override;
    virtual void updateSystem() override;
    virtual void interpolate(float alpha) override;
    void setAcceleration(const Vec3f& dir, float amount);
    static constexpr float DEFAULT_DAMPING = 0.95f;
    const std::shared_ptr<Camera>& getCamera() const;
    void setDamping(float damping);
  private:
    std::shared_ptr<Camera> _camera;
    Vec3f _acceleration = 0.f;
    Vec3f _velocity = 0.f;
    Vec3f _speed;
    float _damping = DEFAULT_DAMPING;
    Vec3f _currentPos, _previousPos;

  };
}

#endif
