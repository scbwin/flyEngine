#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <math/FlyMath.h>
#include "Component.h"

namespace fly
{
  class Transform : public Component
  {
  public:
    Transform(const Vec3f& translation = Vec3f(0.f), const Vec3f& scale = Vec3f(1.f), const Vec3f& degrees = Vec3f(0.f));
    Mat4f getModelMatrix() const;
    void setTranslation(const Vec3f& translation);
    void setScale(const Vec3f& scale);
    void setDegrees(const Vec3f& degrees);
    const Vec3f& getTranslation() const;
    const Vec3f& getScale() const;
    const Vec3f& getDegrees() const;
  private:
    Vec3f _translation;
    Vec3f _scale;
    Vec3f _degrees;


  };
}

#endif // !TRANSFORM_H
