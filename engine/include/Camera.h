#ifndef CAMERA_H
#define CAMERA_H

#include <math/FlyMath.h>
#include "Component.h"
#include <array>
#include <ZNearMapping.h>

namespace fly
{
  enum class IntersectionResult
  {
    OUTSIDE, INSIDE, INTERSECTING
  };

  class AABB;
  class Sphere;

  class Camera : public Component
  {
  public:
    Camera(const Vec3f& pos, const Vec3f& euler_angles);
    Camera(const Camera& other) = default;
    virtual ~Camera();
    Mat4f updateViewMatrix(const Vec3f& pos, const Vec3f& euler_angles);
    Mat3f getViewMatrixInverse() const;
    const Vec3f& getPosition() const;
    const Vec3f& getDirection() const;
    const Vec3f& getRight() const;
    const Vec3f& getUp() const;
    const Vec3f& getEulerAngles() const;
    void setPosition(const Vec3f& position);
    void setEulerAngles(const Vec3f& euler_angles);
    bool isActive() const;
    void setActive(bool active);
    void extractFrustumPlanes(const Mat4f& vp, ZNearMapping z_near_mapping);
    const std::array<Vec4f, 6>& getFrustumPlanes() const;
    IntersectionResult planeIntersectsAABB(const Vec4f& plane, const Vec3f& aabb_half_diagonal, const Vec4f& aabb_center) const;
    IntersectionResult frustumIntersectsAABB(const AABB& aabb) const;
    IntersectionResult planeIntersectsSphere(const Vec4f& plane, const Sphere& sphere) const;
    IntersectionResult frustumIntersectsSphere(const Sphere& sphere) const;
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