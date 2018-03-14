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
    _entities[e.get()] = e;
    return e;
  }
  void EntityManager::removeEntity(Entity* entity)
  {
    _entities.erase(entity);
  }
  void EntityManager::addListener(const std::weak_ptr<System>& listener)
  {
    _listeners.insert(listener);
  }
  void EntityManager::notifyListeners(Entity* entity)
  {
    std::vector<std::weak_ptr<System>> to_delete;
    if (_listeners.size()) {
      for (const auto& l : _listeners) {
        auto listener = l.lock();
        if (listener != nullptr) {
          listener->onComponentsChanged(entity);
        }
        else {
          to_delete.push_back(l);
        }
      }
      for (auto& l : to_delete) {
        _listeners.erase(l);
      }
    }
  }
}