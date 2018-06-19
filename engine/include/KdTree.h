#ifndef KDTREE_H
#define KDTREE_H

#include <vector>
#include <memory>
#include <AABB.h>
#include <Camera.h>
#include <StackPOD.h>
#include <IntersectionTests.h>

namespace fly
{
  template<typename T, typename BV>
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
      BV _bv;
      float _largestBVSize = 0.f;
      inline bool isLargeEnough(const Camera::CullingParams& cp) const
      {
        return _bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize);
      }
    public:
      inline const BV& getBV() const
      {
        return _bv;
      }
      Node(std::vector<T*>& objects, unsigned begin, unsigned end, unsigned depth)
      {
        _left._child = nullptr;
        _right._child = nullptr;
        for (unsigned i = begin; i < end; i++) {
          _bv = _bv.getUnion(objects[i]->getBV());
          _largestBVSize = std::max(_largestBVSize, objects[i]->getLargestObjectBVSize());
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
          unsigned index = _bv.getLongestAxis(depth);
          std::sort(objects.begin() + begin, objects.begin() + end, [index](const T* o1, const T* o2) {
            return o1->getBV().center()[index] > o2->getBV().center()[index];
          });
          auto half_size = size / 2;
          _left._child = new Node(objects, begin, begin + half_size, depth + 1);
          if (half_size > 0) {
            _right._child = new Node(objects, begin + half_size, end, depth + 1);
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
          std::cout << indent << "Leaf aabb:" << _bv << std::endl;
        }
        else {
          std::cout << indent << "Internal aabb:" << _bv << std::endl;
        }
        if (!_isLeaf && _left._child) {
          _left._child->print(level + 1);
        }
        if (!_isLeaf && _right._child) {
          _right._child->print(level + 1);
        }
      }
      void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node*>& nodes)
      {
        if (isLargeEnough(cp)) {
          nodes.push_back_secure(this);
          if (!_isLeaf) {
            _left._child->cullAllNodes(cp, nodes);
            if (_right._child) {
              _right._child->cullAllNodes(cp, nodes);
            }
          }
        }
      }
      void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node*>& nodes)
      {
        if (isLargeEnough(cp)) {
          auto result = IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
          if (result == IntersectionResult::INSIDE) {
            nodes.push_back_secure(this);
            if (!_isLeaf) {
              _left._child->cullAllNodes(cp, nodes);
              if (_right._child) {
                _right._child->cullAllNodes(cp, nodes);
              }
            }
          }
          else if (result == IntersectionResult::INTERSECTING) {
            nodes.push_back_secure(this);
            if (!_isLeaf) {
              _left._child->cullVisibleNodes(cp, nodes);
              if (_right._child) {
                _right._child->cullVisibleNodes(cp, nodes);
              }
            }
          }
        }
      }
      void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T*>& all_objects) const
      {
        if (isLargeEnough(cp)) {
          cullAllObjects2(cp, all_objects);
        }
      }
      inline void cullAllObjects2(const Camera::CullingParams& cp, StackPOD<T*>& all_objects) const
      {
        if (_isLeaf) {
          if (_left._object->cull(cp)) {
            all_objects.push_back(_left._object);
          }
          if (_right._object && _right._object->cull(cp)) {
            all_objects.push_back(_right._object);
          }
        }
        else {
          _left._child->cullAllObjects(cp, all_objects);
          if (_right._child) {
            _right._child->cullAllObjects(cp, all_objects);
          }
        }
      }
      void cullVisibleObjects(const Camera::CullingParams& cp, StackPOD<T*>& visible_objects) const
      {
        if (isLargeEnough(cp)) {
          auto result = IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
          if (result == IntersectionResult::INSIDE) {
            cullAllObjects2(cp, visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            if (_isLeaf) {
              if (_left._object->cullAndIntersect(cp)) {
                visible_objects.push_back(_left._object);
              }
              if (_right._object && _right._object->cullAndIntersect(cp)) {
                visible_objects.push_back(_right._object);
              }
            }
            else {
              _left._child->cullVisibleObjects(cp, visible_objects);
              if (_right._child) {
                _right._child->cullVisibleObjects(cp, visible_objects);
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
      void intersectObjects(const BV& bv, StackPOD<T*>& intersected_objects) const
      {
        if (bv.contains(_bv)) {
          getAllObjects(intersected_objects);
        }
        else if (bv.intersects(_bv)) {
          if (_isLeaf) {
            if (_left._object && bv.intersects(_left._object->getBV())) {
              intersected_objects.push_back_secure(_left._object);
            }
            if (_right._object && bv.intersects(_right._object->getBV())) {
              intersected_objects.push_back_secure(_right._object);
            }
          }
          else {
            _left._child->intersectObjects(bv, intersected_objects);
            if (_right._child) {
              _right._child->intersectObjects(bv, intersected_objects);
            }
          }
        }
      }
    };
    KdTree(std::vector<T*>& objects) :
      _root(objects, 0, static_cast<unsigned>(objects.size()), 0)
    {
    }
    void intersectObjects(const BV& bv, StackPOD<T*>& stack) const
    {
      _root.intersectObjects(bv, stack);
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
      _root.cullVisibleNodes(camera.getCullingParams(), nodes);
    }
    void cullVisibleObjects(const Camera& camera, StackPOD<T*>& visible_objects) const
    {
      _root.cullVisibleObjects(camera.getCullingParams(), visible_objects);
    }
  private:
    Node _root;
  };
}

#endif