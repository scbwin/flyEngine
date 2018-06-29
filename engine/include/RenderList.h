#ifndef RENDERLIST_H
#define RENDERLIST_H

#include <renderer/MeshRenderables.h>
#include <StackPOD.h>

namespace fly
{
  template<typename API, typename BV>
  class RenderList
  {
    using MeshRenderable = IMeshRenderable<API, BV>;
    using Stack = StackPOD<MeshRenderable*>;
  public:
    inline void reserve(size_t size)
    {
      _visibleMeshes.reserve(size);
      _gpuCullList.reserve(size);
      _gpuLodList.reserve(size);
      _cpuLodList.reserve(size);
    }
    inline void clear()
    {
      _visibleMeshes.clear();
      _gpuCullList.clear();
      _gpuLodList.clear();
      _cpuLodList.clear();
    }
    inline size_t size() { return _visibleMeshes.size() + _gpuCullList.size() + _gpuLodList.size() + _cpuLodList.size(); }
    inline size_t capacity() { return _visibleMeshes.capacity() + _gpuCullList.capacity() + _gpuLodList.capacity() + _cpuLodList.capacity(); }
    inline const Stack& getVisibleMeshes() const { return _visibleMeshes; }
    inline const Stack& getGPUCullList() const { return _gpuCullList; }
    inline const Stack& getGPULodList() const { return _gpuLodList; }
    inline const Stack& getCPULodList() const { return _cpuLodList; }
    inline void append(const RenderList& other)
    {
      _visibleMeshes.append(other._visibleMeshes);
      _gpuCullList.append(other._gpuCullList);
      _gpuLodList.append(other._gpuLodList);
      _cpuLodList.append(other._cpuLodList);
    }
    inline void addVisibleMesh(MeshRenderable* mesh) { _visibleMeshes.push_back(mesh); }
    inline void addToGPUCullList(MeshRenderable* mesh) { _gpuCullList.push_back(mesh); }
    inline void addToGPULodList(MeshRenderable* mesh) { _gpuLodList.push_back(mesh); }
    inline void addToCPULodList(MeshRenderable* mesh) { _cpuLodList.push_back(mesh); }
  private:
    Stack _visibleMeshes;
    Stack _gpuCullList;
    Stack _gpuLodList;
    Stack _cpuLodList;
  };
}

#endif