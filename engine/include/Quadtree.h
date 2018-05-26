#ifndef QUADTREE_H
#define QUADTREE_H

#include <array>
#include <math/FlyMath.h>
#include <AABB.h>
#include <memory>
#include <sstream>
#include <Settings.h>
#include <Camera.h>
#include <StackPOD.h>

namespace fly
{
  template<typename T>
  class Quadtree
  {
    // using TPtr = std::shared_ptr<T>;
    using TPtr = T * ;
    using Stack = StackPOD<TPtr>;
  public:
    class Node
    {
    public:
      Node(const Vec2f& min, const Vec2f& max) :
        _min(min),
        _max(max),
        _aabbWorld(),
        _largestElementAABBWorldSize(0.f)
      {
      }
      inline const Vec2f& getMin() const { return _min; }
      inline const Vec2f& getMax() const { return _max; }
      inline Vec2f getSize() const { return _max - _min; }
      inline void setAABBWorld(const AABB& aabb) { _aabbWorld = aabb; }
      inline AABB* getAABBWorld() { return &_aabbWorld; }
      inline const std::vector<TPtr> & getElements() { return _elements; }
      void insert(const TPtr& element)
      {
        AABB const * aabb_element = element->getAABBWorld();
        _aabbWorld = _aabbWorld.getUnion(*aabb_element);
        _largestElementAABBWorldSize = std::max(_largestElementAABBWorldSize, element->getLargestElementAABBSize());
        for (unsigned char i = 0; i < 4; i++) {
          Vec2f child_min, child_max;
          getChildBounds(child_min, child_max, i);
          if (child_min <= aabb_element->getMin().xz() && child_max >= aabb_element->getMax().xz()) { // The child node encloses the element entirely, therefore push it further down the tree.
            if (_children[i] == nullptr) { // Create the node if not yet constructed
              _children[i] = std::make_unique<Node>(child_min, child_max);
            }
            _children[i]->insert(element);
            return;
          }
        }
        _elements.push_back(element); // The element doesn't fit into any of the child nodes, therefore insert it into the current node.
      }
      void print(unsigned level) const
      {
        std::string indent;
        for (unsigned i = 0; i < level; i++) {
          indent += "  ";
        }
        std::cout << indent << "Node bounds:" << _min << " " << getMax() << " extents:" << _aabbWorld.getMin() << " " << _aabbWorld.getMax() << std::endl;
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

      void cullAllElements(Stack& all_elements, const Camera& camera)
      {
        if (_aabbWorld.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          for (const auto& e : _elements) {
            if (e->cull(camera)) {
              all_elements.push_back(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->cullAllElements(all_elements, camera);
            }
          }
        }
      }

      void cullVisibleElements(Stack& visible_elements, const Camera& camera) const
      {
        if (_aabbWorld.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          auto result = camera.intersectFrustumAABB(_aabbWorld);
          if (result == IntersectionResult::INSIDE) {
            for (const auto& e : _elements) {
              if (e->cull(camera)) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->cullAllElements(visible_elements, camera);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            for (const auto& e : _elements) {
              if (e->cullAndIntersect(camera)) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->cullVisibleElements(visible_elements, camera);
              }
            }
          }
        }
      }
      void getAllElements(Stack& all_elements) const
      {
        for (const auto& e : _elements) {
          all_elements.push_back_secure(e);
        }
        for (const auto& c : _children) {
          if (c) {
            c->getAllElements(all_elements);
          }
        }
      }
      void getAllNodes(StackPOD<Node*>& all_nodes)
      {
        all_nodes.push_back_secure(this);
        for (const auto& c : _children) {
          if (c) {
            c->getAllNodes(all_nodes);
          }
        }
      }
      void cullAllNodes(StackPOD<Node*>& nodes, const Camera& camera)
      {
        if (_aabbWorld.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          nodes.push_back_secure(this);
          for (const auto& c : _children) {
            if (c) {
              c->cullAllNodes(nodes, camera);
            }
          }
        }
      }
      void cullVisibleNodes(StackPOD<Node*>& visible_nodes, const Camera& camera)
      {
        if (_aabbWorld.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          auto result = camera.intersectFrustumAABB(_aabbWorld);
          if (result == IntersectionResult::INSIDE) {
            visible_nodes.push_back_secure(this);
            for (const auto& c : _children) {
              if (c) {
                c->cullAllNodes(visible_nodes, camera);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            visible_nodes.push_back_secure(this);
            for (const auto& c : _children) {
              if (c) {
                c->cullVisibleNodes(visible_nodes, camera);
              }
            }
          }
        }
      }
      bool removeElement(const TPtr& element)
      {
        /*for (unsigned i = 0; i < _elements.size(); i++) {
          if (_elements[i] == element) {
            _elements.erase(_elements.begin() + i);
            return true;
          }
        }
        for (const auto& c : _children) {
          if (c && c->removeElement(element)) {
            return true;
          }
        }
        std::cout << "Attempting to remove element from the quadtree that wasn't added. This should never happen." << std::endl;
        return false;*/
        if (element->getAABBWorld()->intersects(_aabbWorld)) {
          for (unsigned i = 0; i < _elements.size(); i++) {
            if (_elements[i] == element) {
              _elements.erase(_elements.begin() + i);
              return true;
            }
          }
          for (const auto& c : _children) {
            if (c && c->removeElement(element)) {
              return true;
            }
          }
        }
        std::cout << "Attempting to remove element from the quadtree that wasn't added. This should never happen." << std::endl;
        return false;
      }
      void intersectElements(const AABB& aabb, Stack& stack)
      {
        if (aabb.contains(_aabbWorld)) {
          for (const auto& e : _elements) {
            stack.push_back_secure(e);
          }
          for (const auto& c : _children) {
            if (c) {
              c->getAllElements(stack);
            }
          }
        }
        else if (aabb.intersects(_aabbWorld)) {
          for (const auto& e : _elements) {
            if (aabb.intersects(*e->getAABBWorld())) {
              stack.push_back_secure(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->intersectElements(aabb, stack);
            }
          }
        }
      }
    private:
      std::unique_ptr<Node> _children[4];
      // Axis aligned bounding box for the enclosed elements (union)
      AABB _aabbWorld;
      // Largest element aabb size that is enclosed by this node, useful for detail culling
      float _largestElementAABBWorldSize;
      // Pointers to the elements
      std::vector<TPtr> _elements;
      // Node min max
      Vec2f _min, _max;
      void getChildBounds(Vec2f& min, Vec2f& max, unsigned char index) const
      {
        auto new_size = getSize() * 0.5f;
        min = _min + Vec2f(static_cast<float>(index % 2), static_cast<float>(index / 2)) * new_size;
        max = min + new_size;
      }
    };

    Quadtree(const AABB& bounds)
    {
      _root = std::make_unique<Node>(bounds.getMin().xz(), bounds.getMax().xz());
      _root->setAABBWorld(bounds);
    }
    void insert(const TPtr& element)
    {
      if (_root->getAABBWorld()->contains(*element->getAABBWorld())) {
        _root->insert(element);
      }
      else {
       /* auto all_elements = getAllElements();
        AABB aabb_new = _root->getAABBWorld()->getUnion(*element->getAABBWorld());
        _root = std::make_unique<Node>(aabb_new.getMin().xz(), aabb_new.getMax().xz());
        _root->setAABBWorld(aabb_new);
        _root->insert(element);
        for (const auto& e : all_elements) {
          _root->insert(e);
        }*/
        std::cout << "error" << std::endl;
      }
    }
    void intersectElements(const AABB& aabb, Stack& stack)
    {
      _root->intersectElements(aabb, stack);
    }
    void print() const
    {
      _root->print(0);
    }
    void cullVisibleElements(const Camera& camera, Stack& stack) const
    {
      _root->cullVisibleElements(stack, camera);
    }
    void getAllElements(Stack& stack) const
    {
      _root->getAllElements(stack);
    }
    void cullVisibleNodes(StackPOD<Node*>& stack, const Camera& camera) const
    {
      _root->cullVisibleNodes(stack, camera);
    }
    void getAllNodes(StackPOD<Node*>& stack)
    {
      _root->getAllNodes(stack);
    }
    bool removeElement(const TPtr& element)
    {
      return _root->removeElement(element);
    }
  private:
    std::unique_ptr<Node> _root;
  };
}

#endif
