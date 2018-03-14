#include "Entity.h"
#include "EntityManager.h"
#include "Component.h"

namespace fly
{
  Entity::Entity(EntityManager* em) : _em(em)
  {
  }
  Entity::~Entity()
  {
    _components.clear();
    _em->notifyListeners(this);
  }
}