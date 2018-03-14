#ifndef FIXEDTIMESTEPSYSTEM_H
#define FIXEDTIMESTEPSYSTEM_H

#include <memory>
#include <System.h>

namespace fly
{
  class Entity;

  class FixedTimestepSystem : public System
  {
  public:
    FixedTimestepSystem() = default;
    virtual ~FixedTimestepSystem() = default;
    virtual void update(float time, float delta_time) override;
    virtual void updateSystem(float time, float delta_time) = 0;
  private:
    float _acc = 0.f;
    float _dt = 1.f / 60.f;
  };
}

#endif
