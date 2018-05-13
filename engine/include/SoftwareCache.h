#ifndef SOFTWARECACHE_H
#define SOFTWARECACHE_H

#include <map>
#include <functional>
#include <vector>

namespace fly
{
  template <typename Key, typename Val, typename... CreateArgs>
  class SoftwareCache
  {
  public:
    SoftwareCache(const std::function<Val(CreateArgs...)>& create_func) : _createFunc(create_func)
    {}
    inline const Val& getOrCreate(const Key& key, CreateArgs... args) {
      auto it = _elements.find(key);
      if (it != _elements.end()) { // Element with this key already exists
        return it->second; // Return the cached element
      }
      _elements[key] = _createFunc(args...); // Create new element
      return _elements[key]; // Return the newly created element
    }
    std::vector<Val> getElements() const
    {
      std::vector<Val> elements;
      for (const auto& e : _elements) {
        elements.push_back(e.second);
      }
      return elements;
    }
    void clear() { _elements.clear(); }
  private:
    std::map<Key, Val> _elements;
    std::function<Val(CreateArgs...)> _createFunc;
  };
}

#endif