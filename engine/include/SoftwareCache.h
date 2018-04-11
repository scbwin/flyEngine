#ifndef SOFTWARECACHE_H
#define SOFTWARECACHE_H

#include <map>
#include <functional>
#include <vector>

namespace fly
{
  template <typename Key, typename Val, typename... Args>
  class SoftwareCache
  {
  public:
    SoftwareCache(const std::function<Val(Args...)>& create_func) : _createFunc(create_func)
    {}
    Val getOrCreate(const Key& key, Args... args) {
      auto it = _elements.find(key);
      if (it != _elements.end()) { // Element with this key already exists
        return it->second;
      }
      auto Val = _createFunc(args...); // Create new element
      _elements[key] = Val; // Add the new element to the cache
      return Val;
    }
    std::vector<Val> getElements()
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
    std::function<Val(Args...)> _createFunc;
  };
}

#endif