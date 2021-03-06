#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <memory>
#include <math/FlyMath.h>

namespace fly
{
  class Camera;

  class CameraController
  {
  public:
    CameraController(const std::shared_ptr<Camera>& camera, float speed);
    virtual ~CameraController() = default;
    void setCamera(const std::shared_ptr<Camera>& camera);
  /*  void stepForward(float delta_time) const;
    void stepBackward(float delta_time) const;
    void stepLeft(float delta_time) const;
    void stepRight(float delta_time) const;
    void stepUp(float delta_time) const;
    void stepDown(float delta_time) const;
    void acceleratePressed();
    void accelerateReleased();
    void deceleratePressed();
    void decelerateReleased();*/
    void mousePress(const Vec3f& mouse_pos);
    void mouseMove(const Vec3f& mouse_pos);
    void mouseRelease();
    void setSpeed(float speed);
    float getSpeed() const;
    bool isPressed() const;
    float getMouseSpeed() const;
    void setMouseSpeed(float speed);
    const std::shared_ptr<Camera>& getCamera() const;
    void setSpeedFactor(float speed_factor);
    float getSpeedFactor() const;
  private:
    std::shared_ptr<Camera> _camera;
    bool _pressed = false;
    float _baseSpeed;
    float _mouseSpeed = 0.01f;
    float _accelerate = 1.f;
    float _speedFactor = 1.f;
    Vec3f _mousePos = Vec3f(0.f);
    void updatePos(const Vec3f& dir, float delta_time) const;
  };
}

#endif
