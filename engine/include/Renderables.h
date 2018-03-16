#ifndef RENDERABLES_H
#define RENDERABLES_H

#include <Component.h>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

namespace fly
{
  class StaticModelRenderable : public Component
  {
  public:
    StaticModelRenderable() = default;
    virtual ~StaticModelRenderable() = default;
  };
  class SkyboxRenderable : public Component
  {
  public:
    SkyboxRenderable() = default;
    virtual ~SkyboxRenderable() = default;
  };
  class ProceduralTerrainRenderable : public Component
  {
  public:
    enum NoiseType
    {
      MultiFractal, RidgedMultiFractal
    };
    ProceduralTerrainRenderable(float frequency, const glm::vec2& uv_frequ, NoiseType noise_type, float height_scale, int num_octaves, const cv::Mat& noise_values, float frequency_scale = 2.f, float amp_scale = 0.5f);
    virtual ~ProceduralTerrainRenderable() = default;
    float getFrequency() const;
    void setFrequency(float frequ);
    float getHeightScale() const;
    void setHeightScale(float height_scale);
    int getNumOctaves() const;
    void setNumOctaves(int num_octaves);
    cv::Mat getNoiseValues();
    float getAmpScale() const;
    void setAmpScale(float amp_scale);
    float getFrequencyScale() const;
    void setFrequencyScale(float frequency_scale);
  private:
    float _frequency;
    glm::vec2 _uvFrequ;
    NoiseType _noiseType;
    float _heightScale;
    int _numOctaves;
    cv::Mat _noiseValues;
    float _ampScale;
    float _frequencyScale;
  };
}

#endif