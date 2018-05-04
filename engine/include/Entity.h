#ifndef ENTITY_H
#define ENTITY_H

#include <map>
#include <memory>
#include <typeindex>
#include <EntityManager.h>

namespace fly
{
  class Component;

  class Entity
  {
  public:
    Entity(EntityManager * const em);
    virtual ~Entity();
    template <typename T>
    void addComponent(const std::shared_ptr<T>& component)
    {
      static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");
      _components[std::type_index(typeid(T))] = component;
      _em->onComponentAdded(this, component);
    }
    template <typename T>
    void removeComponent()
    {
      static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");
      _em->onComponentRemoved(this, _components[std::type_index(typeid(T))]);
      _components.erase(std::type_index(typeid(T)));
    }
    template<typename T>
    std::shared_ptr<T> getComponent()
    {
      static_assert(std::is_base_of<Component, T>::value, "T must be derived from Component");
      std::type_index idx(typeid(T));
      if (_components.count(idx)) {
        return std::dynamic_pointer_cast<T>(_components[idx]);
      }
      return nullptr;
    }
  private:
    std::map<std::type_index, std::shared_ptr<Component>> _components;
    EntityManager* const _em;
  };
}

#endif // !ENTITY_H
