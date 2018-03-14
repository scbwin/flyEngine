#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>

namespace fly
{
  class Entity;

  class System
  {
  public:
    System();
    virtual ~System();

    virtual void onComponentsChanged(Entity* entity) = 0;
    virtual void update(float time, float delta_time) = 0;
  };
}

#endif // ! SYSTEM_H
