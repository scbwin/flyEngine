#ifndef QUADTREE_H
#define QUADTREE_H

#include <array>
#include <math/FlyMath.h>
#include <AABB.h>
#include <memory>
#include <sstream>
#include <Settings.h>

namespace fly
{
  template<typename T>
  class Quadtree
  {
    // using TPtr = std::shared_ptr<T>;
    using TPtr = T * ;
  public:
    class Node
    {
    public:
      Node(const Vec2f& min, const Vec2f& size) :
        _min(min),
        _size(size),
        _aabbWorld(Vec3f(std::numeric_limits<float>::max()), Vec3f(std::numeric_limits<float>::lowest()))
      {
      }
      inline const Vec2f& getMin() const { return _min; }
      inline Vec2f getMax() const { return _min + _size; }
      inline const Vec2f& getSize() const { return _size; }
      inline void setAABBWorld(const AABB& aabb) { _aabbWorld = aabb; }
      inline AABB* getAABBWorld() { return &_aabbWorld; }
      inline bool hasChildren() const
      {
        for (const auto& c : _children) {
          if (c) {
            return true;
          }
        }
        return false;
      }
      void insert(const TPtr& element)
      {
        AABB* aabb_el = element->getAABBWorld();
        _aabbWorld = _aabbWorld.getUnion(*aabb_el);
        std::array<Vec2f, 4> child_min, child_max;
        getChildBounds(child_min, child_max);
        for (unsigned i = 0; i < 4; i++) {
          if (child_min[i] <= Vec2f(aabb_el->getMin()[0], aabb_el->getMin()[2]) && child_max[i] >= Vec2f(aabb_el->getMax()[0], aabb_el->getMax()[2])) { // The child node encloses the element entirely, therefore push it further down the tree.
            if (_children[i] == nullptr) { // Create the node if not yet constructed
              _children[i] = std::make_unique<Node>(child_min[i], _size * 0.5f);
            }
            _children[i]->insert(element);
            return;
          }
        }
        _elements.push_back(element);
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
      template<bool directx>
      void getVisibleElements(const Mat4f& vp, std::vector<TPtr>& visible_elements) const
      {
        if (_elements.size() || hasChildren()) {
          if (_aabbWorld.isFullyVisible<directx>(vp)) {
            visible_elements.insert(visible_elements.end(), _elements.begin(), _elements.end());
            for (const auto& c : _children) {
              if (c) {
                c->getAllElements(visible_elements);
              }
            }
          }
          else if (_aabbWorld.intersectsFrustum<directx>(vp)) {
            for (const auto& e : _elements) {
              if (e->getAABBWorld()->intersectsFrustum<directx>(vp)) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->getVisibleElements<directx>(vp, visible_elements);
              }
            }
          }
        }
      }

      void getAllElementsWithDetailCulling(const Vec3f& cam_pos,
        const DetailCullingParams& detail_culling_params, std::vector<TPtr>& all_elements)
      {
        if (!_aabbWorld.isDetail(cam_pos, detail_culling_params._errorThreshold)) {
          for (const auto& e : _elements) {
            if (!e->getAABBWorld()->isDetail(cam_pos, detail_culling_params._errorThreshold)) {
              all_elements.push_back(e);
            }
          }
          for (const auto& c : _children) {
            if (c) {
              c->getAllElementsWithDetailCulling(cam_pos, detail_culling_params, all_elements);
            }
          }
        }
      }

      template<bool directx>
      void getVisibleElementsWithDetailCulling(const Mat4f& vp, const Vec3f& cam_pos,
        const DetailCullingParams& detail_culling_params, std::vector<TPtr>& visible_elements) const
      {
        if ((_elements.size() || hasChildren()) && !_aabbWorld.isDetail(cam_pos, detail_culling_params._errorThreshold)) {
          if (_aabbWorld.isFullyVisible<directx>(vp)) {
            for (const auto& e : _elements) {
              if (!e->getAABBWorld()->isDetail(cam_pos, detail_culling_params._errorThreshold)) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->getAllElementsWithDetailCulling(cam_pos, detail_culling_params, visible_elements);
              }
            }
          }
          else if (_aabbWorld.intersectsFrustum<directx>(vp)) {
            for (const auto& e : _elements) {
              if (!e->getAABBWorld()->isDetail(cam_pos, detail_culling_params._errorThreshold) && e->getAABBWorld()->intersectsFrustum<directx>(vp)) {
                visible_elements.push_back(e);
              }
            }
            for (const auto& c : _children) {
              if (c) {
                c->getVisibleElementsWithDetailCulling<directx>(vp, cam_pos, detail_culling_params, visible_elements);
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
      template<bool directx, bool ignore_near>
      void getVisibleElementsWithDetailCulling(const std::vector<Mat4f>& vp, const Vec3f& cam_pos,
        const DetailCullingParams& detail_culling_params, std::vector<TPtr>& visible_elements) const
      {
        if ((_elements.size() || hasChildren()) && _aabbWorld.isVisible<directx, ignore_near>(vp)) {
          for (const auto& e : _elements) {
            if (e->getAABBWorld()->isVisible<directx, ignore_near>(vp)) {
              visible_elements.push_back(e);
            }
          }
          float error = (_aabbWorld.getMax() - _aabbWorld.getMin()).length() / (cam_pos - _aabbWorld.center()).length();
          error = pow(error, detail_culling_params._errorExponent);
          if (error > detail_culling_params._errorThreshold) {
            for (const auto& c : _children) {
              if (c) {
                c->getVisibleElementsWithDetailCulling<directx, ignore_near>(vp, cam_pos, detail_culling_params, visible_elements);
              }
            }
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
      template<bool directx>
      void getVisibleNodes(std::vector<Node*>& visible_nodes, const Mat4f& vp)
      {
        if (_aabbWorld.isFullyVisible<directx>(vp)) {
          visible_nodes.push_back(this);
          for (const auto& c : _children) {
            if (c) {
              c->getAllNodes(visible_nodes);
            }
          }
        }
        else if (_aabbWorld.intersectsFrustum<directx>(vp)) {
          visible_nodes.push_back(this);
          for (const auto& c : _children) {
            if (c) {
              c->getVisibleNodes<directx>(visible_nodes, vp);
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
      /**
      * Indices: 0 = South west, 1 = South east, 2 = North east, 3 = North west
      */
      std::unique_ptr<Node> _children[4];
      // Node position
      Vec2f _min;
      // Node size
      Vec2f _size;
      // Axis aligned bounding box for the enclosed elements
      AABB _aabbWorld;
      std::vector<TPtr> _elements;
      void getChildBounds(std::array<Vec2f, 4>& min, std::array<Vec2f, 4>& max) const
      {
        auto new_size = _size * 0.5f;
        min[0] = _min;
        min[1] = _min + Vec2f(new_size[0], 0.f);
        min[2] = _min + new_size;
        min[3] = _min + Vec2f(0.f, new_size[1]);
        max[0] = min[0] + new_size;
        max[1] = min[1] + new_size;
        max[2] = min[2] + new_size;
        max[3] = min[3] + new_size;
      }
    };

    Quadtree(const Vec3f& min, const Vec3f& max)
    {
      Vec2f node_min(min[0], min[2]);
      Vec2f node_max(max[0], max[2]);
      _root = std::make_unique<Node>(node_min, node_max - node_min);
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
        _root = std::make_unique<Node>(Vec2f(aabb_new.getMin()[0], aabb_new.getMin()[2]), Vec2f(aabb_new.getMax()[0] - aabb_new.getMin()[0], aabb_new.getMax()[2] - aabb_new.getMin()[2]));
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
    template<bool directx = false>
    std::vector<TPtr> getVisibleElements(const Mat4f& vp) const
    {
      std::vector<TPtr> visible_elements;
      _root->getVisibleElements<directx>(vp, visible_elements);
      return visible_elements;
    }
    template<bool directx, bool ignore_near>
    std::vector<TPtr> getVisibleElementsWithDetailCulling(const std::vector<Mat4f>& vp, const Vec3f& cam_pos) const
    {
      std::vector<TPtr> visible_elements;
      _root->getVisibleElementsWithDetailCulling<directx, ignore_near>(vp, cam_pos, _detailCullingParams, visible_elements);
      return visible_elements;
    }
    template<bool directx>
    std::vector<TPtr> getVisibleElementsWithDetailCulling(const Mat4f& vp, const Vec3f& cam_pos) const
    {
      std::vector<TPtr> visible_elements;
      _root->getVisibleElementsWithDetailCulling<directx>(vp, cam_pos, _detailCullingParams, visible_elements);
      return visible_elements;
    }
    std::vector<TPtr> getAllElements() const
    {
      std::vector<TPtr> all_elements;
      _root->getAllElements(all_elements);
      return all_elements;
    }
    std::vector<Node*> getAllNodes()
    {
      std::vector<Node*> all_nodes;
      _root->getAllNodes(all_nodes);
      return all_nodes;
    }
    template<bool directx = false>
    std::vector<Node*> getVisibleNodes(const Mat4f& vp)
    {
      std::vector<Node*> visible_nodes;
      _root->getVisibleNodes<directx>(visible_nodes, vp);
      return visible_nodes;
    }
    void setDetailCullingParams(const DetailCullingParams& params)
    {
      _detailCullingParams = params;
    }
    bool removeElement(const TPtr& element)
    {
      return _root->removeElement(element);
    }
  private:
    std::unique_ptr<Node> _root;
    DetailCullingParams _detailCullingParams = { 0.0125f, 1.f };
  };
}

#endif
