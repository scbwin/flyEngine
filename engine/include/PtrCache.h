#ifndef PTRCACHE_H
#define PTRCACHE_H

#include <memory>
#include <map>
#include <functional>
#include <mutex>

namespace fly
{
  template<typename Key, typename Val, typename ... Args>
  class PtrCache
  {
  public:
    PtrCache(const std::function<Val*(Args...)>& creator) : 
      _creator(creator),
      _mutex(std::make_unique<std::mutex>())
    {}
    PtrCache(const PtrCache& other) = delete;
    PtrCache& operator=(const PtrCache& other) = delete;
    PtrCache(PtrCache&& other) :
      _cache(std::move(other._cache)),
      _creator(std::move(other._creator)),
      _mutex(std::move(other._mutex))
    {}
    PtrCache& operator=(PtrCache&& other)
    {
      if (this != &other) {
        _cache = std::move(other._cache);
        _creator = std::move(other._creator);
        _mutex = std::move(other._mutex);
      }
      return *this;
    }
    /**
    * Returns true if ret was taken from the cache, false otherwise.
    */
    bool getOrCreate(const Key& key, std::shared_ptr<Val>& ret, Args... args)
    {
      std::lock_guard<std::mutex> lock(*_mutex);
      auto it = _cache.find(key);
      if (it != _cache.end()) {
        ret = std::shared_ptr<Val>(it->second); // Construct from weak_ptr
        return true;
      }
      ret = std::shared_ptr<Val>(_creator(args...), [this, key](Val* ptr) { // Construct from creator and pass custom deleter which removes the element from the cache.
        std::lock_guard<std::mutex> lock(*_mutex);
        _cache.erase(key);
        delete ptr;
      });
      _cache[key] = ret;
      return false;
    }
    inline void clear()
    {
      std::lock_guard<std::mutex> lock(*_mutex);
      _cache.clear();
    }
    inline size_t size()
    {
      std::lock_guard<std::mutex> lock(*_mutex);
      return _cache.size();
    }
  private:
    std::map<Key, std::weak_ptr<Val>> _cache;
    std::function<Val*(Args...)> _creator;
    std::unique_ptr<std::mutex> _mutex;
  };
}

#endif
