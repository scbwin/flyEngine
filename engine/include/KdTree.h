#ifndef KDTREE_H
#define KDTREE_H

#include <memory>
#include <Camera.h>
#include <vector>
#include <StackPOD.h>
#include <IntersectionTests.h>
#include <CullResult.h>

namespace fly
{
  /**
  * TODO: Make this class even more generic. E.g. one could also store indices instead of pointers
  */
  template<typename T, typename BV, T invalid_value = nullptr>
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
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const = 0;
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const = 0;
      virtual void getSizeInBytes(size_t& bytes) const = 0;
      virtual void intersectObjects(const BV& bv, StackPOD<T>& stack) const = 0;
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const = 0;
      virtual void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const = 0;
    protected:
      BV _bv;
      float _largestBVSize = 0.f;
      inline bool isLargeEnough(const Camera::CullingParams& cp) const
      {
        return _bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize);
      }
      inline IntersectionResult intersect(const Camera::CullingParams& cp) const
      {
        return IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
      }
    };
    class LeafNode : public Node
    {
    public:
      LeafNode(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
        : Node(begin, end, depth, objects)
      {
        _left = objects[begin];
        _right = end - begin > 1 ? objects[begin + 1] : invalid_value;
      }
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersect(cp);
          if (result == IntersectionResult::INSIDE) {
            add(cull_result._fullyVisibleObjects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            add(cull_result._intersectedObjects);
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
          if (isValid(_left) && bv.intersects(_left->getBV())) {
            objects.push_back_secure(_left);
          }
          if (isValid(_right) && bv.intersects(_right->getBV())) {
            objects.push_back_secure(_right);
          }
        }
      }
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp) && intersect(cp) != IntersectionResult::OUTSIDE) {
          nodes.push_back_secure(this);
        }
      }
      virtual void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp)) {
          nodes.push_back_secure(this);
        }
      }
    private:
      T _left;
      T _right;
      inline void add(StackPOD<T>& objects) const
      {
        if (isValid(_left)) {
          objects.push_back(_left);
        }
        if (isValid(_right)) {
          objects.push_back(_right);
        }
      }
      inline bool isValid(const T& t) const { return t != invalid_value; }
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
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersect(cp);
          if (result == IntersectionResult::INSIDE) {
            _left->cullAllObjects(cp, cull_result._fullyVisibleObjects);
            _right->cullAllObjects(cp, cull_result._fullyVisibleObjects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            _left->cullVisibleObjects(cp, cull_result);
            _right->cullVisibleObjects(cp, cull_result);
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
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersect(cp);
          if (result != IntersectionResult::OUTSIDE) {
            nodes.push_back_secure(this);
            if (result == IntersectionResult::INSIDE) {
              _left->cullAllNodes(cp, nodes);
              _right->cullAllNodes(cp, nodes);
            }
            else {
              _left->cullVisibleNodes(cp, nodes);
              _right->cullVisibleNodes(cp, nodes);
            }
          }
        }
      }
      virtual void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp)) {
          nodes.push_back_secure(this);
          _left->cullAllNodes(cp, nodes);
          _right->cullAllNodes(cp, nodes);
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
    void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const
    {
      _root->cullVisibleObjects(cp, cull_result);
    }
    void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const
    {
       _root->intersectObjects(bv, intersected_objects);
    }
    void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes)
    {
      _root->cullVisibleNodes(cp, nodes);
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
    static std::unique_ptr<Node> createNode(unsigned num_objects, unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
    {
      if (num_objects <= 2) {
        return num_objects ? std::make_unique<LeafNode>(begin, end, depth, objects) : nullptr;
      }
      return std::make_unique<InternalNode>(begin, end, depth, objects);
    }
  };
}

#endif
