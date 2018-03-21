#ifndef QUADTREE_H
#define QUADTREE_H

#include <array>
#include <math/FlyMath.h>
#include <AABB.h>
#include <memory>
#include <sstream>

namespace fly
{
  template<typename T>
  class Quadtree
  {
    using TPtr = std::shared_ptr<T>;
  public:
    class Node
    {
    public:
      Node(const Vec2f& min, const Vec2f& size, const Vec3f& bb_min, const Vec3f& bb_max) : _min(min), _size(size)
      {
        _aabbWorld = std::unique_ptr<AABB>(new AABB(bb_min, bb_max));
      }
      const Vec2f& getMin() const { return _min; }
      Vec2f getMax() const { return _min + _size; }
      const Vec2f& getSize() const { return _size; }
      void insert(const TPtr& element)
      {
        AABB* aabb_el = element->getAABBWorld();
        _aabbWorld = std::unique_ptr<AABB>(new AABB(minimum(_aabbWorld->getMin(), aabb_el->getMin()), maximum(_aabbWorld->getMax(), aabb_el->getMax())));
        Vec2f bb_min_el({ aabb_el->getMin()[0], aabb_el->getMin()[2] });
        Vec2f bb_max_el({ aabb_el->getMax()[0], aabb_el->getMax()[2] });
        std::array<Vec2f, 4> child_min, child_max;
        getChildBounds(child_min, child_max);
        for (unsigned i = 0; i < 4; i++) {
          if ((bb_min_el >= child_min[i]) && (bb_max_el <= child_max[i])) { // The node encloses the object entirely, therefore insert it
            if (_children[i] == nullptr) { // Create the node if not yet constructed
              _children[i] = std::make_unique<Node>(child_min[i], _size * 0.5f, aabb_el->getMin(), aabb_el->getMax());
            }
            _children[i]->insert(element);
            return;
          }
        }
        if (bb_min_el >= _min && bb_max_el <= getMax()) {
          _elements.push_back(element);
        }
        else {
          std::stringstream ss;
          ss << "Could not add element. Child bounds:" << bb_min_el << " " << bb_max_el << ", node bounds:" << _min << " " << getMax() << ". Check the bounds of the quadtree.";
          throw std::exception(ss.str().c_str());
        }
      }
      void print(unsigned level) const
      {
        std::string indent;
        for (unsigned i = 0; i < level; i++) {
          indent += "  ";
        }
        std::cout << indent << "Node bounds:" << _min << " " << getMax() << " extents:" << _aabbWorld->getMin() << " " << _aabbWorld->getMax() << std::endl;
        if (_elements.size()) {
          std::cout << indent << "Element aabbs world:" << std::endl;
          for (const auto& e : _elements) {
            std::cout << indent << e->getAABBWorld()->getMin() << " " << e->getAABBWorld()->getMax() << std::endl;
          }
        }
        for (const auto& c : _children) {
          if (c) {
            c->print(level + 1);
          }
        }
      }
    private:
      /**
      * Indices: 0 = South west, 1 = South east, 2 = North east, 3 = North west
      */
      std::array<std::unique_ptr<Node>, 4> _children;
      // Node position
      Vec2f _min;
      // Node size
      Vec2f _size;
      // Axis aligned bounding box for the enclosed elements
      std::unique_ptr<AABB> _aabbWorld;
      std::vector<TPtr> _elements;
      void getChildBounds(std::array<Vec2f, 4>& min, std::array<Vec2f, 4>& max) const
      {
        auto new_size = _size * 0.5f;
        min[0] = _min;
        min[1] = _min + Vec2f({ new_size[0], 0.f });
        min[2] = _min + new_size;
        min[3] = _min + Vec2f({ 0.f, new_size[1] });
        max[0] = min[0] + new_size;
        max[1] = min[1] + new_size;
        max[2] = min[2] + new_size;
        max[3] = min[3] + new_size;
      }
    };
    Quadtree(const Vec2f& min, const Vec2f& max) : _min(min), _size(max - _min)
    {
      _root = std::unique_ptr<Node>(new Node(min, _size, Vec3f(std::numeric_limits<float>::max()), Vec3f(std::numeric_limits<float>::lowest())));
    }
    void insert(const TPtr& element)
    {
      _root->insert(element);
    }
    void print() const
    {
      _root->print(0);
    }
  private:
    Vec2f _min;
    Vec2f _size;
    std::unique_ptr<Node> _root;
  };
}

#endif
