#include <TerrainNew.h>
#include <assert.h>

namespace fly
{
  TerrainNew::TerrainNew(int size, const std::vector<std::string>& albedo_paths, 
    const std::vector<std::string>& normal_paths, float uv_scale_details) :
    _size(size), 
    _albedoPaths(albedo_paths), 
    _normalPaths(normal_paths),
    _uvScaleDetails(uv_scale_details)
  {
  }
  int TerrainNew::getSize() const
  {
    return _size;
  }
  void TerrainNew::setSize(int size)
  {
    _size = size;
  }
  std::vector<std::string> TerrainNew::getAlbedoPaths() const
  {
    return _albedoPaths;
  }
  std::vector<std::string> TerrainNew::getNormalPaths() const
  {
    return _normalPaths;
  }
  float TerrainNew::getUVScaleDetails() const
  {
    return _uvScaleDetails;
  }
  void TerrainNew::setUVScaleDetails(float uv_scale_details)
  {
    _uvScaleDetails = uv_scale_details;
  }
  float TerrainNew::getMaxTessFactor() const
  {
    return _maxTessFactor;
  }
  void TerrainNew::setMaxTessFactor(float tess_factor)
  {
    assert(tess_factor >= 0.f && tess_factor <= 6.f);
    _maxTessFactor = tess_factor;
  }
  float TerrainNew::getMaxTessDistance() const
  {
    return _maxTessDistance;
  }
  void TerrainNew::setMaxTessDistance(float distance)
  {
    _maxTessDistance = distance;
  }
}