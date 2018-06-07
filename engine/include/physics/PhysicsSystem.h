#ifndef PHYSICSSYSTEM_H
#define PHYSICSSYSTEM_H

#include <FixedTimestepSystem.h>
#include <map>
#include <physics/ParticleSystem.h>

namespace fly
{
  class PhysicsSystem : public FixedTimestepSystem
  {
  public:
    PhysicsSystem() = default;
    virtual ~PhysicsSystem() = default;
    virtual void updateSystem() override;

  private:
  };
}

#endif // !PHYSICSSYSTEM_H
