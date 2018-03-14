#ifndef NOISE_GEN_H
#define NOISE_GEN_H

#include <vector>
#include <glm/glm.hpp>

namespace fly
{
  class NoiseGen
  {
  public:
    NoiseGen(int grid_size = 16, bool make_tileable = true);
    virtual ~NoiseGen();
    float getPerlin(const glm::vec2& pos);
  private:
    int _gridWidth;
    std::vector<glm::vec2> _gradientVectors;
    float dotGridGradient(const glm::ivec2& grid_pos, const glm::vec2& pos);
    glm::vec2 smoothstep(const glm::vec2& t);
  };
}

#endif
