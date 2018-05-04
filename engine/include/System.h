#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>
#include <map>

namespace fly
{
  class Entity;
  class Component;

  class System
  {
  public:
    System();
    virtual ~System();

    virtual void onComponentAdded(Entity* entity, const std::shared_ptr<Component>& component) = 0;
    virtual void onComponentRemoved(Entity* entity, const std::shared_ptr<Component>& component) = 0;
    virtual void update(float time, float delta_time) = 0;
  protected:
    template<typename ComponentType>
    std::shared_ptr<ComponentType> addIfInterested(Entity* e, const std::shared_ptr<Component>& component, std::map<Entity*, std::shared_ptr<ComponentType>>& map)
    {
      auto c_interested = e->getComponent<ComponentType>();
      if (c_interested == component) {
        map[e] = c_interested;
        return c_interested;
      }
      return nullptr;
    }
    template<typename ComponentType>
    std::shared_ptr<ComponentType> deleteIfInterested(Entity* e, const std::shared_ptr<Component>& component, std::map<Entity*, std::shared_ptr<ComponentType>>& map)
    {
      auto c_interested = e->getComponent<ComponentType>();
      if (c_interested == component) {
        map.erase(e);
        return c_interested;
      }
      return nullptr;
    }
  };
}

#endif // ! SYSTEM_H
