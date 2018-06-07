#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>
#include <map>

namespace fly
{
  class Entity;
  class Component;
  class GameTimer;

  class System
  {
  public:
    System();
    virtual ~System();
    void setGameTimer(GameTimer const * game_timer);
    virtual void update() = 0;
  protected:
    GameTimer const * _gameTimer;
  };
}

#endif // ! SYSTEM_H
