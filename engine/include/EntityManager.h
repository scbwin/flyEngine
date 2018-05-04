#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <memory>
#include <set>
#include <map>
#include <functional>
#include <vector>

namespace fly
{
  class Entity;
  class System;
  class Component;

  class EntityManager
  {
  public:
    EntityManager() = default;
    std::shared_ptr<Entity> createEntity();
    void removeEntity(const std::shared_ptr<Entity>& entity);
    void addListener(const std::weak_ptr<System>& listener);
    void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component);
    void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component);
    void notifyListeners(Entity* entity, const std::function<void(const std::shared_ptr<System>&)>& notify_func);
   // std::vector<Entity*> getEntities() const;
  private:
   // std::map<Entity*, std::shared_ptr<Entity>> _entities;
    std::set<std::shared_ptr<Entity>> _entities;
    std::set<std::weak_ptr<System>, std::owner_less<std::weak_ptr<System>>> _listeners;
 };
}

#endif // !ENTITYMANAGER_H
