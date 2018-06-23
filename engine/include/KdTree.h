#ifndef KDTREE_H
#define KDTREE_H

#include <memory>
#include <Camera.h>
#include <vector>
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
    public:
      Node(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
      {
        for (unsigned i = begin; i < end; i++) {
          _bv = _bv.getUnion(objects[i]->getBV());
          _largestBVSize = std::max(_largestBVSize, objects[i]->getLargestObjectBVSize());
        }
        unsigned index = _bv.getLongestAxis(depth);
        std::sort(objects.begin() + begin, objects.begin() + end, [index](const T o1, const T o2) {
          return o1->getBV().center()[index] > o2->getBV().center()[index];
        });
      }
      virtual ~Node() = default;
      const BV& getBV() const
      {
        return _bv;
      }
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, StackPOD<T>& fully_visible_objects, StackPOD<T>& intersected_objects) const = 0;
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const = 0;
      virtual void getSizeInBytes(size_t& bytes) const = 0;
      virtual void intersectObjects(const BV& bv, StackPOD<T>& stack) const = 0;
    protected:
      BV _bv;
      float _largestBVSize = 0.f;
      inline bool isLargeEnough(const Camera::CullingParams& cp) const
      {
        return _bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize);
      }
    };
    class LeafNode : public Node
    {
    public:
      LeafNode(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
        : Node(begin, end, depth, objects)
      {
        _left = objects[begin];
        _right = end - begin > 1 ? objects[begin + 1] : nullptr;
      }
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, StackPOD<T>& fully_visible_objects, StackPOD<T>& intersected_objects) const override
      {
        if (isLargeEnough(cp)) {
          auto result = IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
          if (result == IntersectionResult::INSIDE) {
            add(fully_visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            add(intersected_objects);
          }
        }
      }
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const override
      {
        if (isLargeEnough(cp)) {
          add(objects);
        }
      }
      virtual void getSizeInBytes(size_t& bytes) const override
      {
        bytes += sizeof(*this);
      }
      virtual void intersectObjects(const BV& bv, StackPOD<T>& objects) const override
      {
        if (bv.intersects(_bv)) {
          if (_left && bv.intersects(_left->getBV())) {
            objects.push_back_secure(_left);
          }
          if (_right && bv.intersects(_right->getBV())) {
            objects.push_back_secure(_right);
          }
        }
      }
    private:
      T _left;
      T _right;
      inline void add(StackPOD<T>& objects) const
      {
        if (_left) {
          objects.push_back(_left);
        }
        if (_right) {
          objects.push_back(_right);
        }
      }
    };
    class InternalNode : public Node
    {
    public:
      InternalNode(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
        : Node(begin, end, depth, objects)
      {
        unsigned num_objects = end - begin;
        unsigned num_objects_left = num_objects / 2;
        unsigned num_objects_right = num_objects - num_objects_left;
        _left = createNode(num_objects_left, begin, begin + num_objects_left, depth + 1, objects);
        _right = createNode(num_objects_right, begin + num_objects_left, end, depth + 1, objects);
      }
      virtual ~InternalNode() = default;
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, StackPOD<T>& fully_visible_objects, StackPOD<T>& intersected_objects) const override
      {
        if (isLargeEnough(cp)) {
          auto result = IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
          if (result == IntersectionResult::INSIDE) {
            _left->cullAllObjects(cp, fully_visible_objects);
            _right->cullAllObjects(cp, fully_visible_objects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            _left->cullVisibleObjects(cp, fully_visible_objects, intersected_objects);
            _right->cullVisibleObjects(cp, fully_visible_objects, intersected_objects);
          }
        }
      }
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const override
      {
        if (isLargeEnough(cp)) {
          _left->cullAllObjects(cp, objects);
          _right->cullAllObjects(cp, objects);
        }
      }
      virtual void getSizeInBytes(size_t& bytes) const override
      {
        bytes += sizeof(*this);
        _left->getSizeInBytes(bytes);
        _right->getSizeInBytes(bytes);
      }
      virtual void intersectObjects(const BV& bv, StackPOD<T>& objects) const override
      {
        if (bv.intersects(_bv)) {
          _left->intersectObjects(bv, objects);
          _right->intersectObjects(bv, objects);
        }
      }
    private:
      std::unique_ptr<Node> _left;
      std::unique_ptr<Node> _right;
    };
    KdTree(std::vector<T>& objects) :
      _root(createNode(static_cast<unsigned>(objects.size()), 0, static_cast<unsigned>(objects.size()), 0, objects))
    {
    }
    static std::unique_ptr<Node> createNode(unsigned num_objects, unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
    {
      if (num_objects <= 2) {
        return num_objects ? std::make_unique<LeafNode>(begin, end, depth, objects) : nullptr;
      }
      return std::make_unique<InternalNode>(begin, end, depth, objects);
    }
    void cullVisibleObjects(const Camera::CullingParams& cp, StackPOD<T>& fully_visible_objects, StackPOD<T>& intersected_objects) const
    {
      _root->cullVisibleObjects(cp, fully_visible_objects, intersected_objects);
    }
    void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const
    {
       _root->intersectObjects(bv, intersected_objects);
    }
    void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node*>& nodes)
    {

    }
    size_t getSizeInBytes() const
    {
      size_t bytes = 0;
      _root->getSizeInBytes(bytes);
      return bytes;
    }
    const BV& getBV() const
    {
      return _root->getBV();
    }
  private:
    std::unique_ptr<Node> _root;
  };
}

#endif
