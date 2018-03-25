#include <Renderables.h>
#include <Transform.h>
#include <Model.h>

namespace fly
{
  ProceduralTerrainRenderable::ProceduralTerrainRenderable(float frequency, const glm::vec2 & uv_frequ, 
    NoiseType noise_type, float height_scale, int num_octaves, const cv::Mat& noise_values, float frequency_scale, float amp_scale) :
    _frequency(frequency),
    _uvFrequ(uv_frequ), 
    _noiseType(noise_type),
    _heightScale(height_scale),
    _numOctaves(num_octaves),
    _noiseValues(noise_values),
    _frequencyScale(frequency_scale),
    _ampScale(amp_scale)
  {
  }
  float ProceduralTerrainRenderable::getFrequency() const
  {
    return _frequency;
  }
  void ProceduralTerrainRenderable::setFrequency(float frequ)
  {
    _frequency = frequ;
  }
  float ProceduralTerrainRenderable::getHeightScale() const
  {
    return _heightScale;
  }
  void ProceduralTerrainRenderable::setHeightScale(float height_scale)
  {
    _heightScale = height_scale;
  }
  int ProceduralTerrainRenderable::getNumOctaves() const
  {
    return _numOctaves;
  }
  void ProceduralTerrainRenderable::setNumOctaves(int num_octaves)
  {
    _numOctaves = num_octaves;
  }
  cv::Mat ProceduralTerrainRenderable::getNoiseValues()
  {
    return _noiseValues;
  }
  float ProceduralTerrainRenderable::getAmpScale() const
  {
    return _ampScale;
  }
  void ProceduralTerrainRenderable::setAmpScale(float amp_scale)
  {
    _ampScale = amp_scale;
  }
  float ProceduralTerrainRenderable::getFrequencyScale() const
  {
    return _frequencyScale;
  }
  void ProceduralTerrainRenderable::setFrequencyScale(float frequency_scale)
  {
    _frequencyScale = frequency_scale;
  }
  StaticModelRenderable::StaticModelRenderable(const std::vector<std::shared_ptr<Model>>& lods, const std::shared_ptr<Transform>& transform, float lod_divisor) :
    _lods(lods),
    _modelMatrix(transform->getModelMatrix()),
    _maxLod(lods.size() - 1),
    _lodDivisor(lod_divisor)
  {
    auto bb_min = lods[0]->getAABB()->getMin();
    auto bb_max = lods[0]->getAABB()->getMax();
    for (unsigned i = 1; i < lods.size(); i++) {
      bb_min = minimum(bb_min, lods[i]->getAABB()->getMin());
      bb_max = maximum(bb_max, lods[i]->getAABB()->getMax());
    }
    _aabbWorld = std::make_shared<AABB>(AABB(bb_min, bb_max), _modelMatrix);
  }
  const std::shared_ptr<AABB>& StaticModelRenderable::getAABBWorld() const
  {
    return _aabbWorld;
  }
  const Mat4f & StaticModelRenderable::getModelMatrix() const
  {
    return _modelMatrix;
  }
  const std::vector<std::shared_ptr<Model>>& StaticModelRenderable::getLods() const
  {
    return _lods;
  }
  unsigned StaticModelRenderable::selectLod(const Vec3f& cam_pos) const
  {
    float dist_to_cam = (cam_pos - Vec3f({ _modelMatrix[3][0], _modelMatrix[3][1], _modelMatrix[3][2] })).length();
    return std::min(_maxLod, static_cast<unsigned>(dist_to_cam / _lodDivisor));
  }
}