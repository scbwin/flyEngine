#include <Renderables.h>

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
}