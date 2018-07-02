#ifndef RENDERLIST_H
#define RENDERLIST_H

#include <renderer/MeshRenderables.h>
#include <StackPOD.h>

namespace fly
{
  template<typename API, typename BV>
  class RenderList
  {
    using MeshRenderable = IMeshRenderable<API, BV> const;
    using GPURenderable = GPURenderable<API>;
    using Stack = StackPOD<MeshRenderable*>;
    using StackLod = StackPOD<LodRenderable*>;
    using StackGPU = StackPOD<GPURenderable*>;
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
    inline const StackGPU& getGPUCullList() const { return _gpuCullList; }
    inline const StackGPU& getGPULodList() const { return _gpuLodList; }
    inline const StackLod& getCPULodList() const { return _cpuLodList; }
    inline void append(const RenderList& other)
    {
      _visibleMeshes.append(other._visibleMeshes);
      _gpuCullList.append(other._gpuCullList);
      _gpuLodList.append(other._gpuLodList);
      _cpuLodList.append(other._cpuLodList);
    }
    inline void addVisibleMesh(MeshRenderable* renderable) { _visibleMeshes.push_back(renderable); }
    inline void addToGPUCullList(GPURenderable* renderable) { _gpuCullList.push_back(renderable); }
    inline void addToGPULodList(GPURenderable* renderable) { _gpuLodList.push_back(renderable); }
    inline void addToCPULodList(LodRenderable* renderable) { _cpuLodList.push_back(renderable); }
  private:
    Stack _visibleMeshes;
    StackGPU _gpuCullList;
    StackGPU _gpuLodList;
    StackLod _cpuLodList;
  };
}

#endif