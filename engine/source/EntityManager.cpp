#include "EntityManager.h"
#include "Entity.h"
#include <vector>
#include "System.h"
#include <iostream>

namespace fly
{
  std::shared_ptr<Entity> EntityManager::createEntity()
  {
    auto e = std::make_shared<Entity>(this);
    _entities.insert(e);
    return e;
  }
  void EntityManager::removeEntity(const std::shared_ptr<Entity>& entity)
  {
   _entities.erase(entity);
  }
  void EntityManager::addListener(const std::weak_ptr<System>& listener)
  {
    _listeners.insert(listener);
  }
  void EntityManager::onComponentAdded(Entity * entity, const std::shared_ptr<Component>& component)
  {
    notifyListeners(entity, [entity, component](const std::shared_ptr<System>& listener) {
      listener->onComponentAdded(entity, component);
    });
  }
  void EntityManager::onComponentRemoved(Entity * entity, const std::shared_ptr<Component>& component)
  {
    notifyListeners(entity, [entity, component](const std::shared_ptr<System>& listener) {
      listener->onComponentRemoved(entity, component);
    });
  }
  void EntityManager::notifyListeners(Entity* entity, const std::function<void(const std::shared_ptr<System>&)>& notify_func)
  {
    std::vector<std::weak_ptr<System>> to_delete;
    if (_listeners.size()) {
      for (const auto& l : _listeners) {
        auto listener = l.lock();
        listener ? notify_func(listener) : to_delete.push_back(l);
      }
      for (auto& l : to_delete) {
        _listeners.erase(l);
      }
    }
  }
}