#include "Entity.h"
#include "EntityManager.h"
#include "Component.h"
#include <iostream>

namespace fly
{
  Entity::Entity(EntityManager* const em) : _em(em)
  {
  }
  Entity::~Entity()
  {
    for (const auto& e : _components) {
      _em->onComponentRemoved(this, e.second);
    }
  }
}