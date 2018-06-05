#ifndef KDTREE_H
#define KDTREE_H

#include <vector>
#include <memory>
#include <AABB.h>
#include <Camera.h>
#include <StackPOD.h>

namespace fly
{
  template<typename T>
  class KdTree
  {
  public:
    class Node
    {
    private:
      union NodeData
      {
        Node* _child;
        T* _object;
      };
      NodeData _left;
      NodeData _right;
      bool _isLeaf = false;
      AABB _aabb;
      float _largestAABBSize = 0.f;
      inline bool isLargeEnough(const Camera& camera) const
      {
        return _aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize);
      }
    public:
      inline const AABB& getAABB() const
      {
        return _aabb;
      }
      Node(std::vector<T*>& objects, unsigned begin, unsigned end)
      {
        _left._child = nullptr;
        _right._child = nullptr;
        for (unsigned i = begin; i < end; i++) {
          _aabb = _aabb.getUnion(objects[i]->getAABB());
          _largestAABBSize = std::max(_largestAABBSize, objects[i]->getLargestObjectAABBSize());
        }
        unsigned size = end - begin;
        if (size <= 2) {
          _isLeaf = true;
          _left._object = objects[begin];
          if (size == 2) {
            _right._object = objects[begin + 1];
          }
        }
        else {
          auto vec = _aabb.getMax() - _aabb.getMin();
          float longest_axis = std::max(vec[0], std::max(vec[1], vec[2]));
          unsigned index;
          if (longest_axis == vec[0]) {
            index = 0;
          }
          else if (longest_axis == vec[1]) {
            index = 1;
          }
          else {
            index = 2;
          }
          std::sort(objects.begin() + begin, objects.begin() + end, [index](const T* o1, const T* o2) {
            return o1->getAABB().center()[index] > o2->getAABB().center()[index];
          });
          auto half_size = size / 2;
          _left._child = new Node(objects, begin, begin + half_size);
          if (half_size > 0) {
            _right._child = new Node(objects, begin + half_size, end);
          }
        }
      }
      ~Node()
      {
        if (!_isLeaf) {
          if (_left._child) {
            delete _left._child;
          }
          if (_right._child) {
            delete _right._child;
          }
        }
      }
      void print(unsigned level)
      {
        std::string indent;
        for (unsigned i = 0; i < level; i++) {
          indent += "  ";
        }
        if (_isLeaf) {
          std::cout << indent << "Leaf aabb:" << _aabb << std::endl;
        }
        else {
          std::cout << indent << "Internal aabb:" << _aabb << std::endl;
        }
        if (!_isLeaf && _left._child) {
          _left._child->print(level + 1);
        }
        if (!_isLeaf && _right._child) {
          _right._child->print(level + 1);
        }
      }
      void cullAllNodes(const Camera& camera, StackPOD<Node*>& nodes)
      {
        if (isLargeEnough(camera)) {
          nodes.push_back_secure(this);
          if (!_isLeaf) {
            _left._child->cullAllNodes(camera, nodes);
            if (_right._child) {
              _right._child->cullAllNodes(camera, nodes);
            }
          }
        }
      }
      void cullVisibleNodes(const Camera& camera, StackPOD<Node*>& nodes)
      {
        if (isLargeEnough(camera)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            nodes.push_back_secure(this);
            if (!_isLeaf) {
              _left._child->cullAllNodes(camera, nodes);
              if (_right._child) {
                _right._child->cullAllNodes(camera, nodes);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            nodes.push_back_secure(this);
            if (!_isLeaf) {
              _left._child->cullVisibleNodes(camera, nodes);
              if (_right._child) {
                _right._child->cullVisibleNodes(camera, nodes);
              }
            }
          }
        }
      }
      void cullAllObjects(const Camera& camera, StackPOD<T*>& all_objects) const
      {
        if (isLargeEnough(camera)) {
          cullAllObjects2(camera, all_objects);
        }
      }
      inline void cullAllObjects2(const Camera& camera, StackPOD<T*>& all_objects) const
      {
        if (_isLeaf) {
          if (_left._object->cull(camera)) {
            all_objects.push_back(_left._object);
          }
          if (_right._object && _right._object->cull(camera)) {
            all_objects.push_back(_right._object);
          }
        }
        else {
          _left._child->cullAllObjects(camera, all_objects);
          if (_right._child) {
            _right._child->cullAllObjects(camera, all_objects);
          }
        }
      }
      void cullVisibleObjects(const Camera& camera, StackPOD<T*>& visible_objects) const
      {
        if (isLargeEnough(camera)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            cullAllObjects2(camera, visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            if (_isLeaf) {
              if (_left._object->cullAndIntersect(camera)) {
                visible_objects.push_back(_left._object);
              }
              if (_right._object && _right._object->cullAndIntersect(camera)) {
                visible_objects.push_back(_right._object);
              }
            }
            else {
              _left._child->cullVisibleObjects(camera, visible_objects);
              if (_right._child) {
                _right._child->cullVisibleObjects(camera, visible_objects);
              }
            }
          }
        }
      }
      void getAllObjects(StackPOD<T*>& all_objects) const
      {
        if (_isLeaf) {
          all_objects.push_back_secure(_left._object);
          if (_right._object) {
            all_objects.push_back_secure(_right._object);
          }
        }
        else {
          _left._child->getAllObjects(all_objects);
          if (_right._child) {
            _right._child->getAllObjects(all_objects);
          }
        }
      }
      void intersectObjects(const AABB& aabb, StackPOD<T*>& intersected_objects) const
      {
        if (aabb.contains(_aabb)) {
          getAllObjects(intersected_objects);
        }
        else if (aabb.intersects(_aabb)) {
          if (_isLeaf) {
            if (_left._object && aabb.intersects(_left._object->getAABB())) {
              intersected_objects.push_back_secure(_left._object);
            }
            if (_right._object && aabb.intersects(_right._object->getAABB())) {
              intersected_objects.push_back_secure(_right._object);
            }
          }
          else {
            _left._child->intersectObjects(aabb, intersected_objects);
            if (_right._child) {
              _right._child->intersectObjects(aabb, intersected_objects);
            }
          }
        }
      }
    };
    KdTree(std::vector<T*>& objects) :
      _root(objects, 0, static_cast<unsigned>(objects.size()))
    {
    }
    void intersectObjects(const AABB& aabb, StackPOD<T*>& stack) const
    {
      _root.intersectObjects(aabb, stack);
    }
    bool removeObject(const T* object)
    {
      // TODO: implement
      return true;
    }
    void print()
    {
      _root.print(0);
    }
    void cullVisibleNodes(const Camera& camera, StackPOD<Node*>& nodes)
    {
      _root.cullVisibleNodes(camera, nodes);
    }
    void cullVisibleObjects(const Camera& camera, StackPOD<T*>& visible_objects) const
    {
      _root.cullVisibleObjects(camera, visible_objects);
    }
  private:
    Node _root;
  };
}

#endif