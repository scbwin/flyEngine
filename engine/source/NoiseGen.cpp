#define GLM_ENABLE_EXPERIMENTAL
#include <NoiseGen.h>
#include <random>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

namespace fly
{
  NoiseGen::NoiseGen(int grid_size, bool make_tileable) : _gridWidth(grid_size + 1)
  {
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist(-1.f, 1.f);

    _gradientVectors.resize(pow(_gridWidth, 2));

    for (int x = 0; x <= grid_size; x++) {
      for (int y = 0; y <= grid_size; y++) {
        _gradientVectors[x + y * _gridWidth] = normalize(glm::vec2(dist(gen), dist(gen)));
      }
    }

   if (make_tileable) {
      for (int x = 0; x <= grid_size; x++) {
        int y = grid_size;
        _gradientVectors[x + y * _gridWidth] = _gradientVectors[x + 0 * _gridWidth];
      }

      for (int y = 0; y <= grid_size; y++) {
        int x = grid_size;
        _gradientVectors[x + y * _gridWidth] = _gradientVectors[0 + y * _gridWidth];
      }
      _gradientVectors[grid_size + grid_size * _gridWidth] = _gradientVectors[0];
    }
  }
  NoiseGen::~NoiseGen()
  {
  }
  float NoiseGen::getPerlin(const glm::vec2& pos)
  {
    glm::ivec2 pos_start = glm::floor(pos);
    glm::ivec2 pos_end = pos_start + 1;
    glm::vec2 weights = smoothstep(pos - glm::vec2(pos_start));
    return glm::mix(glm::mix(dotGridGradient(pos_start, pos), dotGridGradient(glm::ivec2(pos_end.x, pos_start.y), pos), weights.x),
      glm::mix(dotGridGradient(glm::ivec2(pos_start.x, pos_end.y), pos), dotGridGradient(pos_end, pos), weights.x), weights.y);
  }

  float NoiseGen::dotGridGradient(const glm::ivec2 & grid_pos, const glm::vec2 & pos)
  {
    glm::vec2 dist_vec = pos - glm::vec2(grid_pos);
    glm::ivec2 idx_2d(grid_pos.x % _gridWidth, grid_pos.y % _gridWidth);
    int idx = idx_2d.x + idx_2d.y * _gridWidth;
    return glm::dot(dist_vec, _gradientVectors[idx]);
  }

  glm::vec2 NoiseGen::smoothstep(const glm::vec2& t)
  {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
  }
}