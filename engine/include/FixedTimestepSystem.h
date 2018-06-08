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
    virtual void update() override;
    virtual void saveState() = 0;
    virtual void updateSystem() = 0;
    virtual void interpolate(float alpha) = 0;
  private:
    float _acc = 0.f;
  protected:
    float _dt = 1.f / 60.f;
  };
}

#endif
