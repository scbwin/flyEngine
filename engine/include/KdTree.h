#ifndef KDTREE_H
#define KDTREE_H

#include <memory>
#include <Camera.h>
#include <vector>
#include <StackPOD.h>
#include <IntersectionTests.h>
#include <CullResult.h>
#include <boost/pool/object_pool.hpp>

#define KD_TREE_USE_BOOST 1

namespace fly
{
  /**
  * TODO: Make this class even more generic. E.g. one could also store indices instead of pointers
  */
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
        auto axis = _bv.getLongestAxis(depth);
        std::sort(&objects.front() + begin, &objects.front() + end, [&axis](const T& o1, const T& o2) {
          return o1->getBV().center(axis) > o2->getBV().center(axis);
        });
      }
      virtual ~Node() = default;
      const BV& getBV() const { return _bv; }
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const = 0;
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const = 0;
      virtual void getSizeInBytes(size_t& bytes) const = 0;
      virtual void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const = 0;
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const = 0;
      virtual void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const = 0;
    protected:
      BV _bv;
      float _largestBVSize = 0.f;
      inline bool isLargeEnough(const Camera::CullingParams& cp) const
      {
        return _bv.isLargeEnough(cp._camPos, cp._thresh, _largestBVSize);
      }
      inline IntersectionResult intersectFrustum(const Camera::CullingParams& cp) const
      {
        return IntersectionTests::frustumIntersectsBoundingVolume(_bv, cp._frustumPlanes);
      }
    };
    class LeafNodeSingle : public Node
    {
    public:
      LeafNodeSingle(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
        : Node(begin, end, depth, objects)
      {
        _left = objects[begin];
      }
      virtual ~LeafNodeSingle() = default;
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const override
      {
        cull_result._probablyVisibleObjects.push_back(_left);
      }
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const override
      {
        objects.push_back(_left);
      }
      virtual void getSizeInBytes(size_t& bytes) const override
      {
        bytes += sizeof(*this);
      }
      virtual void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const override
      {
        if (bv.intersects(_left->getBV())) {
          intersected_objects.push_back_secure(_left);
        }
      }
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp) && intersectFrustum(cp) != IntersectionResult::OUTSIDE) {
          nodes.push_back_secure(this);
        }
      }
      virtual void cullAllNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp)) {
          nodes.push_back_secure(this);
        }
      }
    protected:
      T _left;
    };
    class LeafNode : public LeafNodeSingle
    {
    public:
      LeafNode(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
        : LeafNodeSingle(begin, end, depth, objects)
      {
        _right = objects[begin + 1];
      }
      virtual void getSizeInBytes(size_t& bytes) const override
      {
        bytes += sizeof(*this);
      }
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersectFrustum(cp);
          if (result == IntersectionResult::INSIDE) {
            add(cull_result._fullyVisibleObjects);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            add(cull_result._probablyVisibleObjects);
          }
        }
      }
      virtual void cullAllObjects(const Camera::CullingParams& cp, StackPOD<T>& objects) const override
      {
        if (isLargeEnough(cp)) {
          add(objects);
        }
      }
      virtual void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const override
      {
        if (bv.intersects(_bv)) {
          if (bv.intersects(_left->getBV())) {
            intersected_objects.push_back_secure(_left);
          }
          if (bv.intersects(_right->getBV())) {
            intersected_objects.push_back_secure(_right);
          }
        }
      }
    private:
      inline void add(StackPOD<T>& objects) const
      {
        objects.push_back(_left);
        objects.push_back(_right);
      }
      T _right;
    };
#if KD_TREE_USE_BOOST
    class NodePool; // Forward declare NodePool class.
    using NodePtr = Node * ;
#define POOL_ARG , NodePool& node_pool
#define POOL_PARAM , node_pool
#define POOL_OBJECT node_pool.
#define POOL_MEMBER , _nodePool
#define POOL_MEMBER_OBJECT _nodePool.
#else
    using NodePtr = std::unique_ptr<Node>;
#define POOL_ARG
#define POOL_PARAM
#define POOL_OBJECT
#define POOL_MEMBER
#define POOL_MEMBER_OBJECT
#endif
    class InternalNode : public Node
    {
    public:
      InternalNode(unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects POOL_ARG)
        : Node(begin, end, depth, objects)
      {
        unsigned num_objects = end - begin;
        unsigned num_objects_left = num_objects / 2;
        unsigned num_objects_right = num_objects - num_objects_left;
        _left = POOL_OBJECT createNode(num_objects_left, begin, begin + num_objects_left, depth + 1, objects POOL_PARAM);
        _right = POOL_OBJECT createNode(num_objects_right, begin + num_objects_left, end, depth + 1, objects POOL_PARAM);
      }
      virtual ~InternalNode() = default;
      virtual void cullVisibleObjects(const Camera::CullingParams& cp, CullResult<T>& cull_result) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersectFrustum(cp);
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
      virtual void intersectObjects(const BV& bv, StackPOD<T>& intersected_objects) const override
      {
        if (bv.intersects(_bv)) {
          _left->intersectObjects(bv, intersected_objects);
          _right->intersectObjects(bv, intersected_objects);
        }
      }
      virtual void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const override
      {
        if (isLargeEnough(cp)) {
          auto result = intersectFrustum(cp);
          if (result == IntersectionResult::INSIDE) {
            nodes.push_back_secure(this);
            _left->cullAllNodes(cp, nodes);
            _right->cullAllNodes(cp, nodes);
          }
          else if (result == IntersectionResult::INTERSECTING) {
            nodes.push_back_secure(this);
            _left->cullVisibleNodes(cp, nodes);
            _right->cullVisibleNodes(cp, nodes);
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
      NodePtr _left, _right;
    };
    KdTree(std::vector<T>& objects) :
       _root(POOL_MEMBER_OBJECT createNode(static_cast<unsigned>(objects.size()), 0, static_cast<unsigned>(objects.size()), 0, objects POOL_MEMBER))
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
    void cullVisibleNodes(const Camera::CullingParams& cp, StackPOD<Node const *>& nodes) const
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
#if KD_TREE_USE_BOOST
    NodePool _nodePool;
    class NodePool
    {
    public:
      NodePool() = default;
      NodePool(const NodePool& other) = delete;
      NodePool& operator=(const NodePool& other) = delete;
      NodePool(NodePool&& other) = delete;
      NodePool& operator=(NodePool&& other) = delete;
      NodePtr createNode(unsigned num_objects, unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects, NodePool& node_pool)
      {
        switch (num_objects) {
        case 2:
          return ::new(_leafNodePool.malloc()) LeafNode(begin, end, depth, objects);
        case 1:
          return ::new(_leafNodeSinglePool.malloc()) LeafNodeSingle(begin, end, depth, objects);
        default:
          return ::new(_internalNodePool.malloc()) InternalNode(begin, end, depth, objects, *this);
        }
      }
    private:
      boost::object_pool<InternalNode> _internalNodePool;
      boost::object_pool<LeafNode> _leafNodePool;
      boost::object_pool<LeafNodeSingle> _leafNodeSinglePool;
    };
#else
    static std::unique_ptr<Node> createNode(unsigned num_objects, unsigned begin, unsigned end, unsigned depth, std::vector<T>& objects)
    {
      assert(num_objects);
      switch (num_objects) {
      case 2:
        return std::make_unique<LeafNode>(begin, end, depth, objects);
      case 1:
        return std::make_unique<LeafNodeSingle>(begin, end, depth, objects);
      default:
        return std::make_unique<InternalNode>(begin, end, depth, objects);
      }
    }
#endif
    NodePtr _root;
  };
}

#endif
