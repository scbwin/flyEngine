#ifndef TERRAINNEW_H
#define TERRAINNEW_H

#include <vector>

namespace fly
{
  class TerrainNew
  {
  public:
    TerrainNew(int size, const std::vector<std::string>& albedo_paths, const std::vector<std::string>& normal_paths, float uv_scale_details);
    virtual ~TerrainNew() = default;
    int getSize() const;
    void setSize(int size);
    std::vector<std::string> getAlbedoPaths() const;
    std::vector<std::string> getNormalPaths() const;
    float getUVScaleDetails() const;
    void setUVScaleDetails(float uv_scale_details);
    float getMaxTessFactor() const;
    void setMaxTessFactor(float tess_factor);
    float getMaxTessDistance() const;
    void setMaxTessDistance(float distance);
  private:
    int _size;
    std::vector<std::string> _albedoPaths;
    std::vector<std::string> _normalPaths;
    float _uvScaleDetails;
    float _maxTessFactor = 6.f;
    float _maxTessDistance = 6192.f;
  };
}

#endif
