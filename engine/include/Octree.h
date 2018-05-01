#ifndef OCTREE_H
#define OCTREE_H

#include <array>
#include <math/FlyMath.h>
#include <AABB.h>
#include <memory>
#include <sstream>
#include <Settings.h>
#include <Camera.h>

namespace fly
{
  template<typename T>
  class Octree
  {
    // using TPtr = std::shared_ptr<T>;
    using TPtr = T * ;
  public:
    class Node
    {
    public:
      Node(const Vec3f& min, const Vec3f& max) :
        _min(min),
        _max(max),
        _aabbWorld(Vec3f(std::numeric_limits<float>::max()), Vec3f(std::numeric_limits<float>::lowest())),
        _largestElementAABBWorldSize(0.f)
      {
      }
      inline const Vec3f& getMin() const { return _min; }
      inline const Vec3f& getMax() const { return _max; }
      inline Vec3f getSize() const { return _max - _min; }
      inline void setAABBWorld(const AABB& aabb) { _aabbWorld = aabb; }
      inline AABB* getAABBWorld() { return &_aabbWorld; }
      void insert(const TPtr& element)
      {
        AABB* aabb_element = element->getAABBWorld();
        _aabbWorld = _aabbWorld.getUnion(*aabb_element);
        _largestElementAABBWorldSize = std::max(_largestElementAABBWorldSize, aabb_element->size());
        for (unsigned char i = 0; i < 8; i++) {
          Vec3f child_min, child_max;
          getChildBounds(child_min, child_max, i);
          if (child_min <= aabb_element->getMin() && child_max >= aabb_element->getMax()) { // The child node encloses the element entirely, therefore push it further down the tree.
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
      inline void getVisibleElements(std::vector<TPtr>& visible_elements, const Camera& camera) const
      {
        auto result = camera.intersectFrustumAABB(_aabbWorld);
        if (result == IntersectionResult::INSIDE) {
          visible_elements.insert(visible_elements.end(), _elements.begin(), _elements.end());
          for (const auto& c : _children) {
            if (c) {
              c->getAllElements(visible_elements);
            }
          }
        }
        else if (result == IntersectionResult::INTERSECTING) {
          for (const auto& e : _elements) {
            if (camera.intersectFrustumAABB(*e->getAABBWorld()) != IntersectionResult::OUTSIDE) {
              visible_elements.push_back(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->getVisibleElements(visible_elements, camera);
            }
          }
        }
      }

      void getAllElementsWithDetailCulling(std::vector<TPtr>& all_elements, const Camera& camera)
      {
        if (!_aabbWorld.isDetail(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          for (const auto& e : _elements) {
            if (!e->getAABBWorld()->isDetail(camera.getPosition(), camera.getDetailCullingThreshold())) {
              all_elements.push_back(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->getAllElementsWithDetailCulling(all_elements, camera);
            }
          }
        }
      }

      inline void getVisibleElementsWithDetailCulling(std::vector<TPtr>& visible_elements, const Camera& camera) const
      {
        if (!_aabbWorld.isDetail(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          auto result = camera.intersectFrustumAABB(_aabbWorld);
          if (result == IntersectionResult::INSIDE) {
            for (const auto& e : _elements) {
              if (!e->getAABBWorld()->isDetail(camera.getPosition(), camera.getDetailCullingThreshold())) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->getAllElementsWithDetailCulling(visible_elements, camera);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            for (const auto& e : _elements) {
              if (!e->getAABBWorld()->isDetail(camera.getPosition(), camera.getDetailCullingThreshold()) && camera.intersectFrustumAABB(*e->getAABBWorld()) != IntersectionResult::OUTSIDE) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->getVisibleElementsWithDetailCulling(visible_elements, camera);
              }
            }
          }
        }
      }
      void getAllElements(std::vector<TPtr>& all_elements) const
      {
        all_elements.insert(all_elements.end(), _elements.begin(), _elements.end());
        for (const auto& c : _children) {
          if (c) {
            c->getAllElements(all_elements);
          }
        }
      }
      void getAllNodes(std::vector<Node*>& all_nodes)
      {
        all_nodes.push_back(this);
        for (const auto& c : _children) {
          if (c) {
            c->getAllNodes(all_nodes);
          }
        }
      }
      inline void getAllNodesWithDetailCulling(std::vector<Node*>& nodes, const Camera& camera)
      {
        if (!_aabbWorld.isDetail(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          nodes.push_back(this);
          for (const auto& c : _children) {
            if (c) {
              c->getAllNodesWithDetailCulling(nodes, camera);
            }
          }
        }
      }
      inline void getVisibleNodesWithDetailCulling(std::vector<Node*>& visible_nodes, const Camera& camera)
      {
        if (!_aabbWorld.isDetail(camera.getPosition(), camera.getDetailCullingThreshold(), _largestElementAABBWorldSize)) {
          auto result = camera.intersectFrustumAABB(_aabbWorld);
          if (result == IntersectionResult::INSIDE) {
            visible_nodes.push_back(this);
            for (const auto& c : _children) {
              if (c) {
                c->getAllNodesWithDetailCulling(visible_nodes, camera);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            visible_nodes.push_back(this);
            for (const auto& c : _children) {
              if (c) {
                c->getVisibleNodesWithDetailCulling(visible_nodes, camera);
              }
            }
          }
        }
      }
      inline void getVisibleNodes(std::vector<Node*>& visible_nodes, const Camera& camera)
      {
        auto result = camera.intersectFrustumAABB(_aabbWorld);
        if (result == IntersectionResult::INSIDE) {
          visible_nodes.push_back(this);
          for (const auto& c : _children) {
            if (c) {
              c->getAllNodes(visible_nodes);
            }
          }
        }
        else if (result == IntersectionResult::INTERSECTING) {
          visible_nodes.push_back(this);
          for (const auto& c : _children) {
            if (c) {
              c->getVisibleNodes(visible_nodes, camera);
            }
          }
        }
      }
      bool removeElement(const TPtr& element)
      {
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
        std::cout << "Attempting to remove element from the quadtree that wasn't added. This should never happen." << std::endl;
        return false;
      }
    private:
      std::unique_ptr<Node> _children[8];
      // Node min max
      Vec3f _min, _max;
      // Axis aligned bounding box for the enclosed elements (union)
      AABB _aabbWorld;
      // Largest element aabb size that is enclosed by this node, useful for detail culling
      float _largestElementAABBWorldSize;
      // Pointers to the elements
      std::vector<TPtr> _elements;
      void getChildBounds(Vec3f& min, Vec3f& max, unsigned char index) const
      {
        auto new_size = getSize() * 0.5f;
        min = _min + Vec3f(static_cast<float>(index % 3), static_cast<float>((index / 3) % 3), static_cast<float>(index / (3 * 3))) * new_size;
        max = min + new_size;
      }
    };

    Octree(const Vec3f& min, const Vec3f& max)
    {
      _root = std::make_unique<Node>(min, max);
      _root->setAABBWorld(AABB(min, max));
    }
    void insert(const TPtr& element)
    {
      if (_root->getAABBWorld()->contains(*element->getAABBWorld())) {
        _root->insert(element);
      }
      else {
        auto all_elements = getAllElements();
        AABB aabb_new = _root->getAABBWorld()->getUnion(*element->getAABBWorld());
        _root = std::make_unique<Node>(aabb_new.getMin(), aabb_new.getMax());
        _root->setAABBWorld(aabb_new);
        _root->insert(element);
        for (const auto& e : all_elements) {
          _root->insert(e);
        }
      }
    }
    void print() const
    {
      _root->print(0);
    }
    std::vector<TPtr> getVisibleElements(const Camera& camera) const
    {
      std::vector<TPtr> visible_elements;
      _root->getVisibleElements(visible_elements, camera);
      return visible_elements;
    }
    std::vector<TPtr> getVisibleElementsWithDetailCulling(const Camera& camera) const
    {
      std::vector<TPtr> visible_elements;
      _root->getVisibleElementsWithDetailCulling(visible_elements, camera);
      return visible_elements;
    }
    inline std::vector<TPtr> getAllElements() const
    {
      std::vector<TPtr> all_elements;
      _root->getAllElements(all_elements);
      return all_elements;
    }
    inline std::vector<Node*> getAllNodes()
    {
      std::vector<Node*> all_nodes;
      _root->getAllNodes(all_nodes);
      return all_nodes;
    }

    inline std::vector<Node*> getVisibleNodesWithDetailCulling(const Camera& camera) const
    {
      std::vector<Node*> visible_nodes;
      _root->getVisibleNodesWithDetailCulling(visible_nodes, camera);
      return visible_nodes;
    }

    inline std::vector<Node*> getVisibleNodes(const Camera& camera)
    {
      std::vector<Node*> visible_nodes;
      _root->getVisibleNodes(visible_nodes, camera);
      return visible_nodes;
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
