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
    struct Node
    {
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
      inline const AABB& getAABB() const
      {
        return _aabb;
      }
      Node(std::vector<T*>& objects)
      {
        _left._child = nullptr;
        _right._child = nullptr;
        for (const auto& o : objects) {
          _aabb = _aabb.getUnion(o->getAABB());
          _largestAABBSize = std::max(_largestAABBSize, o->getLargestObjectAABBSize());
        }
        if (objects.size() <= 2) {
          _isLeaf = true;
          _left._object = objects[0];
          if (objects.size() == 2) {
            _right._object = objects[1];
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
          std::sort(objects.begin(), objects.end(), [index](const T* o1, const T* o2) {
            auto center1 = o1->getAABB().center();
            auto center2 = o2->getAABB().center();
            return center1[index] > center2[index];
          });
          auto half_size = objects.size() / 2;
          std::vector<T*> objects_left(objects.begin(), objects.begin() + half_size);
          std::vector<T*> objects_right(objects.begin() + half_size, objects.end());
          _left._child = new Node(objects_left);
          _right._child = new Node(objects_right);
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
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold())) {
          nodes.push_back_secure(this);
          if (!_isLeaf && _left._child) {
            _left._child->cullAllNodes(camera, nodes);
          }
          if (!_isLeaf && _right._child) {
            _right._child->cullAllNodes(camera, nodes);
          }
        }
      }
      void cullVisibleNodes(const Camera& camera, StackPOD<Node*>& nodes)
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            nodes.push_back_secure(this);
            if (!_isLeaf && _left._child) {
              _left._child->cullAllNodes(camera, nodes);
            }
            if (!_isLeaf && _right._child) {
              _right._child->cullAllNodes(camera, nodes);
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            nodes.push_back_secure(this);
            if (!_isLeaf && _left._child) {
              _left._child->cullVisibleNodes(camera, nodes);
            }
            if (!_isLeaf && _right._child) {
              _right._child->cullVisibleNodes(camera, nodes);
            }
          }
        }
      }
      /* void cullAllObjects(const Camera& camera, Stack& all_objects)
       {
         if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestObjectAABBSize)) {
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
       }*/
      void cullAllObjects(const Camera& camera, StackPOD<T*>& all_objects) const
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize)) {
          cullAllObjects2(camera, all_objects);
        }
      }
      inline void cullAllObjects2(const Camera& camera, StackPOD<T*>& all_objects) const
      {
        if (_isLeaf) {
          if (_left._object && _left._object->cull(camera)) {
            all_objects.push_back(_left._object);
          }
          if (_right._object && _right._object->cull(camera)) {
            all_objects.push_back(_right._object);
          }
        }
        else {
          if (_left._child) {
            _left._child->cullAllObjects(camera, all_objects);
          }
          if (_right._child) {
            _right._child->cullAllObjects(camera, all_objects);
          }
        }
      }
      void cullVisibleObjects(const Camera& camera, StackPOD<T*>& visible_objects) const
      {
        if (_aabb.isLargeEnough(camera.getPosition(), camera.getDetailCullingThreshold(), _largestAABBSize)) {
          auto result = camera.frustumIntersectsAABB(_aabb);
          if (result == IntersectionResult::INSIDE) {
            cullAllObjects2(camera, visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            if (_isLeaf) {
              if (_left._object && _left._object->cullAndIntersect(camera)) {
                visible_objects.push_back(_left._object);
              }
              if (_right._object && _right._object->cullAndIntersect(camera)) {
                visible_objects.push_back(_right._object);
              }
            }
            else {
              if (_left._child) {
                _left._child->cullVisibleObjects(camera, visible_objects);
              }
              if (_right._child) {
                _right._child->cullVisibleObjects(camera, visible_objects);
              }
            }
          }
        }
      }
    };
    KdTree(std::vector<T*>& objects) :
      _root(objects)
    {
    }
    void intersectObjects(const AABB& aabb, StackPOD<T*>& stack)
    {
    }
    bool removeObject(T* object)
    {
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