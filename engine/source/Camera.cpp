#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <AABB.h>

namespace fly
{
  Camera::Camera(const Vec3f& pos, const Vec3f& euler_angles) : _pos(pos), _eulerAngles(euler_angles)
  {
  }

  Camera::~Camera()
  {
  }

  Mat4f Camera::getViewMatrix(const Vec3f& pos, const Vec3f& euler_angles)
  {
    _direction = Vec3f(cos(euler_angles[1]) * sin(euler_angles[0]), sin(euler_angles[1]), cos(euler_angles[1]) * cos(euler_angles[0]));
    _right = Vec3f(sin(euler_angles[0] - glm::half_pi<float>()), sin(euler_angles[2]), cos(euler_angles[0] - glm::half_pi<float>()));
    _up = cross(glm::vec3(_right), glm::vec3(_direction));

    Vec3f target = pos + _direction;

    return glm::lookAt(glm::vec3(pos), glm::vec3(target), glm::vec3(_up));
  }
  const Vec3f & Camera::getPosition() const
  {
    return _pos;
  }
  const Vec3f & Camera::getDirection() const
  {
    return _direction;
  }
  const Vec3f & Camera::getRight() const
  {
    return _right;
  }
  const Vec3f & Camera::getUp() const
  {
    return _up;
  }
  const Vec3f & Camera::getEulerAngles() const
  {
    return _eulerAngles;
  }
  void Camera::setPosition(const Vec3f & position)
  {
    _pos = position;
  }
  void Camera::setEulerAngles(const Vec3f & euler_angles)
  {
    _eulerAngles = euler_angles;
  }
  bool Camera::isActive() const
  {
    return _isActive;
  }
  void Camera::setActive(bool active)
  {
    _isActive = active;
  }
  const std::array<Vec4f, 6>& Camera::extractFrustumPlanes(const Mat4f & vp, bool directx)
  {
    _frustumPlanes[0] = (vp.row(3) + vp.row(0)) * -1.f; // left plane
    _frustumPlanes[1] = (vp.row(3) - vp.row(0)) * -1.f; // right plane
    _frustumPlanes[2] = (vp.row(3) + vp.row(1)) * -1.f; // bottom plane
    _frustumPlanes[3] = (vp.row(3) - vp.row(1)) * -1.f; // top plane
    _frustumPlanes[4] = (directx ? vp.row(2) : (vp.row(3) + vp.row(2))) * -1.f; // near plane
    _frustumPlanes[5] = (vp.row(3) - vp.row(2)) * -1.f; // far plane

    return _frustumPlanes;
  }
  IntersectionResult Camera::intersectPlaneAABB(const Vec4f & plane, const AABB & aabb) const
  {
    auto c = (aabb.getMin() + aabb.getMax()) * 0.5f;
    auto h = (aabb.getMax() - aabb.getMin()) * 0.5f;
    auto e = dot(h, abs(plane.xyz()));
    auto s = dot(Vec4f(c, 1.f), plane);
    if (s - e > 0) {
      return IntersectionResult::OUTSIDE;
    }
    if (s + e < 0) {
      return IntersectionResult::INSIDE;
    }
    return IntersectionResult::INTERSECTING;
  }
  IntersectionResult Camera::intersectFrustumAABB(const AABB & aabb) const
  {
    bool intersecting = false;
    for (const auto& p : _frustumPlanes) {
      auto result = intersectPlaneAABB(p, aabb);
      if (result == IntersectionResult::OUTSIDE) {
        return IntersectionResult::OUTSIDE;
      }
      else if (result == IntersectionResult::INTERSECTING) {
        intersecting = true;
      }
    }
    if (intersecting) {
      return IntersectionResult::INTERSECTING;
    }
    else {
      return IntersectionResult::INSIDE;
    }
  }
  float Camera::getDetailCullingThreshold() const
  {
    return _detailCullingThreshold;
  }
  void Camera::setDetailCullingThreshold(float threshold)
  {
    _detailCullingThreshold = std::max(0.f, threshold);
  }
}