#ifndef CAMERA_H
#define CAMERA_H

#include <math/FlyMath.h>
#include "Component.h"

namespace fly
{
  class Camera : public Component
  {
  public:
    Camera(const Vec3f& pos, const Vec3f& euler_angles);
    virtual ~Camera();
    Mat4f getViewMatrix(const Vec3f& pos, const Vec3f& euler_angles);
    const Vec3f& getPosition() const;
    const Vec3f& getDirection() const;
    const Vec3f& getRight() const;
    const Vec3f& getUp() const;
    const Vec3f& getEulerAngles() const;
    void setPosition(const Vec3f& position);
    void setEulerAngles(const Vec3f& euler_angles);
    bool isActive() const;
    void setActive(bool active);
  private:
    Vec3f _pos;
    Vec3f _eulerAngles;
    Vec3f _right;
    Vec3f _direction;
    Vec3f _up;
    bool _isActive = true;
  };
}

#endif