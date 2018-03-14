#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <memory>
#include <set>
#include <map>

namespace fly
{
  class Entity;
  class System;

  class EntityManager
  {
  public:
    EntityManager() = default;
    std::shared_ptr<Entity> createEntity();
    void removeEntity(Entity* entity);
    void addListener(const std::weak_ptr<System>& listener);
    void notifyListeners(Entity* entity);
  private:
    std::map<Entity*, std::shared_ptr<Entity>> _entities;
    std::set<std::weak_ptr<System>, std::owner_less<std::weak_ptr<System>>> _listeners;
 };
}

#endif // !ENTITYMANAGER_H
