#ifndef CAMERA_H
#define CAMERA_H

#include <math/FlyMath.h>
#include "Component.h"
#include <array>

namespace fly
{
  enum IntersectionResult
  {
    OUTSIDE, INSIDE, INTERSECTING
  };

  class AABB;

  class Camera : public Component
  {
  public:
    Camera(const Vec3f& pos, const Vec3f& euler_angles);
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
    void extractFrustumPlanes(const Mat4f& vp, bool directx = false);
    const std::array<Vec4f, 6>& getFrustumPlanes() const;
    IntersectionResult intersectPlaneAABB(const Vec4f& plane, const Vec3f& h, const Vec4f& center) const;
    IntersectionResult intersectFrustumAABB(const AABB& aabb) const;
    float getDetailCullingThreshold() const;
    void setDetailCullingThreshold(float threshold);
  private:
    Vec3f _pos;
    Vec3f _eulerAngles;
    Vec3f _right;
    Vec3f _direction;
    Vec3f _up;
    std::array<Vec4f, 6> _frustumPlanes;
    bool _isActive = true;
    float _detailCullingThreshold = 0.000175f;
  };
}

#endif