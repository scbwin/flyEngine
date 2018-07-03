#ifndef KDTREE_H
#define KDTREE_H

#include <memory>
#include <Camera.h>
#include <vector>
#include <StackPOD.h>
#include <IntersectionTests.h>
#include <CullResult.h>
#include <boost/pool/object_pool.hpp>

/** 
* Using boost object pools should be used in general. It ensures that
* BVH nodes are placed in contiguous memory chunks. This significantly
* reduces memory fragmentation which leads to improved cache locality and
* better performance.
*/
#define KD_TREE_USE_BOOST 1

namespace fly
{
  /**
  * kd-tree implementation that doesn't store points, but objects associated with bounding volumes.
  * It is a special case of binary space partitioning (BSP) trees, where the splitting planes are 
  * always parallel to one of main coordinate axes. It is used for Hierarchical View Frustum Culling and Detail Culling
  * (see cullVisibleObjects()) and for coarse collision detection algorithms (see intersectObjects()). The
  * tree is built once in the constructor by passing a number of objects of type T (pointer type), associated with a bounding
  * volume of type BV. Dynamic node insertion/removal is currently not supported, because this type of tree can easily become
  * unbalanced.
  * TODO: Make this class even more generic. E.g. one could also store indices instead of pointers.
  * TODO: Support for asynchronous streaming of nodes and individual objects from disk / database.
  */
  template<typename T, typename BV>
  class KdTree
  {
  public:
    class Node
    {
    public:
      Node(unsigned begin, unsigned end, std::vector<T>& objects)
      {
        for (unsigned i = begin; i < end; i++) {
          _bv = _bv.getUnion(objects[i]->getBV());
          _largestBVSize = std::max(_largestBVSize, objects[i]->getLargestObjectBVSize());
        }
        auto axis = _bv.getLongestAxis();
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
      virtual void countNodes(unsigned& internal_nodes, unsigned& leaf_nodes) const = 0;
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
      LeafNodeSingle(unsigned begin, unsigned end, std::vector<T>& objects)
        : Node(begin, end, objects)
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
      virtual void countNodes(unsigned& internal_nodes, unsigned& leaf_nodes) const override
      {
        leaf_nodes += 1;
      }
    protected:
      T _left;
    };
    class LeafNode : public LeafNodeSingle
    {
    public:
      LeafNode(unsigned begin, unsigned end, std::vector<T>& objects)
        : LeafNodeSingle(begin, end, objects)
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
    using NodePtr = Node * ;
#else
    using NodePtr = std::unique_ptr<Node>;
#endif
    class InternalNode : public Node
    {
    public:
      InternalNode(unsigned begin, unsigned end, std::vector<T>& objects, KdTree& kd_tree)
        : Node(begin, end, objects)
      {
        unsigned num_objects = end - begin;
        unsigned num_objects_left = num_objects / 2u;
        _left = kd_tree.createNode(begin, begin + num_objects_left, objects);
        _right = kd_tree.createNode(begin + num_objects_left, end, objects);
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
      virtual void countNodes(unsigned& internal_nodes, unsigned& leaf_nodes) const override
      {
        internal_nodes += 1;
        _left->countNodes(internal_nodes, leaf_nodes);
        _right->countNodes(internal_nodes, leaf_nodes);
      }
    private:
      NodePtr _left, _right;
    };
    KdTree(std::vector<T>& objects) :
#if KD_TREE_USE_BOOST
      _nodePool(static_cast<unsigned>(objects.size())),
#endif
       _root(createNode(0, static_cast<unsigned>(objects.size()), objects))
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
    void countNodes(unsigned& internal_nodes, unsigned& leaf_nodes) const
    {
      internal_nodes = 0;
      leaf_nodes = 0;
      _root->countNodes(internal_nodes, leaf_nodes);
    }
  private:
#if KD_TREE_USE_BOOST
    class NodePool
    {
    public:
      NodePool(unsigned num_objects) :
        _height(static_cast<unsigned>(std::floor(log2(static_cast<double>(num_objects))))),
        _numNodes(static_cast<unsigned>(std::pow(2.0, static_cast<double>(_height) + 1.0) - 1.0)),
        _numLeafNodes(static_cast<unsigned>(std::pow(2.0, static_cast<double>(_height)))),
        _numInternalNodes(_numNodes - _numLeafNodes),
        _internalNodePool(_numInternalNodes),
        _leafNodePool(std::max(_numLeafNodes / 2u, 1u)),
        _leafNodeSinglePool(std::max(_numLeafNodes / 2u, 1u))
      {
      }
      NodePool(const NodePool& other) = delete;
      NodePool& operator=(const NodePool& other) = delete;
      NodePool(NodePool&& other) = delete;
      NodePool& operator=(NodePool&& other) = delete;
      NodePtr createNode(unsigned begin, unsigned end, std::vector<T>& objects, KdTree& kd_tree)
      {
        auto num_objects = end - begin;
        switch (num_objects) {
        case 2:
          return ::new(_leafNodePool.malloc()) LeafNode(begin, end, objects);
        case 1:
          return ::new(_leafNodeSinglePool.malloc()) LeafNodeSingle(begin, end, objects);
        default:
          return ::new(_internalNodePool.malloc()) InternalNode(begin, end, objects, kd_tree);
        }
      }
    private:
      unsigned _height;
      unsigned _numNodes;
      unsigned _numLeafNodes;
      unsigned _numInternalNodes;
      boost::object_pool<InternalNode> _internalNodePool;
      boost::object_pool<LeafNode> _leafNodePool;
      boost::object_pool<LeafNodeSingle> _leafNodeSinglePool;
    };
    NodePool _nodePool;
    inline NodePtr createNode(unsigned begin, unsigned end, std::vector<T>& objects)
    {
      return _nodePool.createNode(begin, end, objects, *this);
    }
#else
    inline NodePtr createNode(unsigned begin, unsigned end, std::vector<T>& objects)
    {
      auto num_objects = end - begin;
      switch (num_objects) {
      case 2:
        return std::make_unique<LeafNode>(begin, end, objects);
      case 1:
        return std::make_unique<LeafNodeSingle>(begin, end, objects);
      default:
        return std::make_unique<InternalNode>(begin, end, objects, *this);
      }
    }
#endif
    NodePtr _root;
  };
}

#endif
