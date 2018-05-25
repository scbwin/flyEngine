#include "System.h"

namespace fly
{
  System::System()
  {
  }
  System::~System()
  {
  }
  void System::setGameTimer(GameTimer const * game_timer)
  {
    _gameTimer = game_timer;
  }
}