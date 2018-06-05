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
#include <Sphere.h>

namespace fly
{
  /**
  * Quadtree implementation that is used for intersection tests and view frustum culling.
  * In order to insert and remove an object, objects of type T must provide the methods getAABB() and getLargestObjectAABBSize() (for instanced geometry).
  * In order to perform visibility tests, objects of type T must provide the methods cull() and cullAndIntersect()
  * that are used for fine-grained view frustum and detail culling.
  * Objects are always stored in the smallest node that encloses the object entirely. The quadtree is sparse, i.e.
  * nodes are only created on demand as soon as objects are added.
  * The quadtree is not responsible for memory management, it can just store raw pointers to the objects.
  */
  template<typename T>
  class Quadtree
  {
    // using TPtr = std::shared_ptr<T>;
    using TPtr = T *;
    using Stack = StackPOD<TPtr>;
  public:
    class Node
    {
    public:
      Node(const Vec2f& min, const Vec2f& max) :
        _min(min),
        _max(max),
        _aabb(),
        _largestObjectAABBSize(0.f)
      {
      }
      inline const Vec2f& getMin() const { return _min; }
      inline const Vec2f& getMax() const { return _max; }
      inline Vec2f getSize() const { return _max - _min; }
      inline void setAABB(const AABB& aabb) { _aabb = aabb; }
      inline const AABB&  getAABB() const { return _aabb; }
      inline const std::vector<TPtr> & getObjects()  const { return _objects; }
      void insert(const TPtr& object)
      {
        const auto& aabb_object = object->getAABB();
        _aabb = _aabb.getUnion(aabb_object);
        _largestObjectAABBSize = std::max(_largestObjectAABBSize, object->getLargestObjectAABBSize());
        for (unsigned char i = 0; i < 4; i++) {
          Vec2f child_min, child_max;
          getChildBounds(child_min, child_max, i);
          if (child_min <= aabb_object.getMin().xz() && child_max >= aabb_object.getMax().xz()) { // The child node encloses the object entirely, therefore push it further down the tree.
            if (_children[i] == nullptr) { // Create the node if not yet constructed
              _children[i] = std::make_unique<Node>(child_min, child_max);
            }
            _children[i]->insert(object);
            return;
          }
        }
        _objects.push_back(object); // The object doesn't fit into any of the child nodes, therefore insert it into the current node.
      }
      bool removeObject(const TPtr& object)
      {
        // TODO: update node aabbs, largest object size and delete nodes if necessary.
        if (object->getAABB().intersects(_aabb)) {
          for (unsigned i = 0; i < _objects.size(); i++) {
            if (_objects[i] == object) {
              _objects.erase(_objects.begin() + i);
              return true;
            }
          }
          for (const auto& c : _children) {
            if (c && c->removeObject(object)) {
              return true;
            }
          }
        }
        std::cout << "Attempting to remove object from the quadtree that wasn't added. This should never happen." << std::endl;
        return false;
      }
      void print(unsigned level) const
      {
        std::string indent;
        for (unsigned i = 0; i < level; i++) {
          indent += "  ";
        }
        std::cout << indent << "Node bounds:" << _min << " " << getMax() << " extents:" << _aabb.getMin() << " " << _aabb.getMax() << std::endl;
        if (_objects.size()) {
          std::cout << indent << "Object aabbs :" << std::endl;
          for (const auto& e : _objects) {
            std::cout << indent << e->getAABB().getMin() << " " << e->getAABB().getMax() << std::endl;
          }
        }
        for (const auto& c : _children) {
          if (c) {
            c->print(level + 1);
          }
        }
      }
      void cullAllObjects2(const Camera& camera, Stack& all_objects) const
      {
        for (const auto& e : _objects) {
          if (e->cull(camera)) { // The node is already large enough -> check if the object is large enough to render.
            all_objects.push_back(e);
          }
        }
        for (const auto& c : _children) {
          if (c) {
            c->cullAllObjects(camera, all_objects);
          }
        }
      }
      void cullAllObjects(const Camera& camera, Stack& all_objects) const
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestObjectAABBSize)) {
          cullAllObjects2(camera, all_objects);
        }
      }
      void cullVisibleObjects(const Camera& camera, Stack& visible_objects) const
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestObjectAABBSize)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            cullAllObjects2(camera, visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            for (const auto& e : _objects) {
              if (e->cullAndIntersect(camera)) { // The node is already large enough and intersects the view frustum -> check if the object is large enough and intersects the view frustum.
                visible_objects.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->cullVisibleObjects(camera, visible_objects); // Proceed with visibility tests for child nodes.
              }
            }
          }
          // No need to process child nodes if the node is outside the view frustum.
        }
        // No need to process child nodes if the node is too small.
      }
      void getAllObjects(Stack& all_objects) const
      {
        for (const auto& e : _objects) {
          all_objects.push_back_secure(e);
        }
        for (const auto& c : _children) {
          if (c) {
            c->getAllObjects(all_objects);
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
      void cullAllNodes(const Camera& camera, StackPOD<Node*>& nodes)
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestObjectAABBSize)) {
          nodes.push_back_secure(this);
          for (const auto& c : _children) {
            if (c) {
              c->cullAllNodes(camera, nodes);
            }
          }
        }
      }
      void cullVisibleNodes(const Camera& camera, StackPOD<Node*>& visible_nodes)
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestObjectAABBSize)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            visible_nodes.push_back_secure(this);
            for (const auto& c : _children) {
              if (c) {
                c->cullAllNodes(camera, visible_nodes);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            visible_nodes.push_back_secure(this);
            for (const auto& c : _children) {
              if (c) {
                c->cullVisibleNodes(camera, visible_nodes);
              }
            }
          }
        }
      }
      void intersectObjects(const AABB& aabb, Stack& intersected_objects)
      {
        if (aabb.contains(_aabb)) {
          for (const auto& e : _objects) {
            intersected_objects.push_back_secure(e);
          }
          for (const auto& c : _children) {
            if (c) {
              c->getAllObjects(intersected_objects);
            }
          }
        }
        else if (aabb.intersects(_aabb)) {
          for (const auto& e : _objects) {
            if (aabb.intersects(e->getAABB())) {
              intersected_objects.push_back_secure(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->intersectObjects(aabb, intersected_objects);
            }
          }
        }
      }
    private:
      // Pointers to child nodes, any of them may be nullptr.
      std::unique_ptr<Node> _children[4];
      // Axis aligned bounding box that the encloses the objects within this node.
      AABB _aabb;
      /** 
      * Largest object aabb size that is enclosed by this node. If the largest object within
      * a node is too small to render, then all the other objects within this node are too small as well
      * and the node can safely be discarded from rendering.
      */
      float _largestObjectAABBSize;
      // Pointers to the objects
      std::vector<TPtr> _objects;
      // Node min max
      Vec2f _min, _max;
      void getChildBounds(Vec2f& min, Vec2f& max, unsigned char index) const
      {
        auto new_size = getSize() * 0.5f;
        min = _min + Vec2f(static_cast<float>(index % 2), static_cast<float>(index / 2)) * new_size;
        max = min + new_size;
      }
    };

    Quadtree(const std::vector<T*>& objects)
    {
      AABB aabb;
      for (const auto& o : objects) {
        aabb = aabb.getUnion(o->getAABB());
      }
      _root = std::make_unique<Node>(aabb.getMin().xz(), aabb.getMax().xz());
      _root->setAABB(aabb);
      for (const auto& o : objects) {
        _root->insert(o);
      }
    }
    void insert(const TPtr& object)
    {
      if (_root->getAABB().contains(object->getAABB())) {
        _root->insert(object);
      }
      else {
        throw std::exception("The object does not fit into the quadtree, please check the bounds of the object and the quadtree");
      }
    }
    void intersectObjects(const AABB& aabb, Stack& stack)
    {
      _root->intersectObjects(aabb, stack);
    }
    void print() const
    {
      _root->print(0);
    }
    void cullVisibleObjects(const Camera& camera, Stack& stack) const
    {
      _root->cullVisibleObjects(camera, stack);
    }
    void getAllObjects(Stack& stack) const
    {
      _root->getAllObjects(stack);
    }
    void cullVisibleNodes(const Camera& camera, StackPOD<Node*>& stack) const
    {
      _root->cullVisibleNodes(camera, stack);
    }
    void getAllNodes(StackPOD<Node*>& stack)
    {
      _root->getAllNodes(stack);
    }
    bool removeObject(const TPtr& object)
    {
      return _root->removeObject(object);
    }
  private:
    std::unique_ptr<Node> _root;
  };
}

#endif
