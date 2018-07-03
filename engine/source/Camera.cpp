#include "Camera.h"
#include <AABB.h>
#include <Sphere.h>
#include <math/MathHelpers.h>

namespace fly
{
  Camera::Camera(const Vec3f& pos, const Vec3f& euler_angles) : 
    _pos(pos), 
    _eulerAngles(euler_angles),
    _params{0.1f, 10000.f, 45.f}
  {
  }

  Camera::~Camera()
  {
  }

  const Mat4f & Camera::updateViewMatrix()
  {
    return updateViewMatrix(_pos, _eulerAngles);
  }

  const Mat4f& Camera::updateViewMatrix(const Vec3f& pos, const Vec3f& euler_angles)
  {
    _direction = Vec3f(cos(euler_angles[1]) * sin(euler_angles[0]), sin(euler_angles[1]), cos(euler_angles[1]) * cos(euler_angles[0]));
    _right = Vec3f(sin(euler_angles[0] - glm::half_pi<float>()), sin(euler_angles[2]), cos(euler_angles[0] - glm::half_pi<float>()));
    _up = cross(glm::vec3(_right), glm::vec3(_direction));

    _viewMatrix = MathHelpers::getViewMatrixLeftHanded(pos, _right, _up, _direction);
    return _viewMatrix;
  }
  const Mat4f & Camera::getViewMatrix()
  {
    return _viewMatrix;
  }
  Mat3f Camera::getViewMatrixInverse() const
  {
    return Mat3f({_right, _up, _direction});
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
  void Camera::extractFrustumPlanes(const Mat4f & vp, ZNearMapping z_near_mapping)
  {
    _frustumPlanes[0] = (vp.row(3) + vp.row(0)) * -1.f; // left plane
    _frustumPlanes[1] = (vp.row(3) - vp.row(0)) * -1.f; // right plane
    _frustumPlanes[2] = (vp.row(3) + vp.row(1)) * -1.f; // bottom plane
    _frustumPlanes[3] = (vp.row(3) - vp.row(1)) * -1.f; // top plane
    _frustumPlanes[4] = (z_near_mapping == ZNearMapping::ZERO ? vp.row(2) : (vp.row(3) + vp.row(2))) * -1.f; // near plane
    _frustumPlanes[5] = (vp.row(3) - vp.row(2)) * -1.f; // far plane

    for (auto& p : _frustumPlanes) {
      p /= p.xyz().length();
    }
  }
  const std::array<Vec4f, 6>& Camera::getFrustumPlanes() const
  {
    return _frustumPlanes;
  }
  float Camera::getDetailCullingThreshold() const
  {
    return _detailCullingThreshold;
  }
  void Camera::setDetailCullingThreshold(float threshold)
  {
    _detailCullingThreshold = std::max(0.000005f, threshold);
  }
  float Camera::getLodRangeMultiplier() const
  {
    return _lodRangeMultiplier;
  }
  void Camera::setLodRangeMultiplier(float multiplier)
  {
    _lodRangeMultiplier = std::max(multiplier, 1.f);
  }
  Camera::CullingParams Camera::getCullingParams() const
  {
    return { _pos, _detailCullingThreshold, _frustumPlanes, (_detailCullingThreshold * _lodRangeMultiplier) - _detailCullingThreshold };
  }
  const Camera::Params & Camera::getParams() const
  {
    return _params;
  }
  void Camera::setParams(const Params & params)
  {
    _params = params;
  }
}