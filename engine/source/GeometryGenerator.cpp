#include <GeometryGenerator.h>

#include <Windows.h>
#include <string>
namespace fly
{
  void GeometryGenerator::generateGrid(int size, std::vector<glm::vec2>& vertices, std::vector<unsigned>& indices)
  {
    assert(!vertices.size() && !indices.size());
    for (int y = 0; y <= size; y++) {
      for (int x = 0; x <= size; x++) {
        vertices.push_back(glm::vec2(x, y));
      }
    }
    auto getIndex = [size](int x, int y) {
      return y * (size + 1) + x;
    };
    for (int y = 0; y + 1 <= size; y++) {
      for (int x = 0; x + 1 <= size; x++) {
        indices.push_back(getIndex(x, y));
        indices.push_back(getIndex(x, y + 1));
        indices.push_back(getIndex(x + 1, y));
        indices.push_back(getIndex(x + 1, y));
        indices.push_back(getIndex(x, y + 1));
        indices.push_back(getIndex(x + 1, y + 1));
      }
    }
  }
  void GeometryGenerator::generateGeoMipMap(int size, int tile_size, unsigned num_lods, std::vector<glm::vec2>& vertices, std::vector<unsigned>& indices, std::map<unsigned, std::map<unsigned, IndexBufferInfo>>& index_offsets)
  {
    assert(!vertices.size() && !indices.size() && size % tile_size == 0);
    std::vector<unsigned> foo;
    generateGrid(tile_size, vertices, foo);
    for (unsigned lod = 0; lod < num_lods; lod++) {
      for (unsigned flag = 0; flag <= 0b1111; flag++) {
        auto new_indices = genGMMIndices(tile_size, lod, flag);
        index_offsets[lod][flag] = { static_cast<unsigned>(new_indices.size()), static_cast<unsigned>(indices.size()) };
        indices.insert(indices.end(), new_indices.begin(), new_indices.end());
      }
    }
  }
  std::vector<unsigned> GeometryGenerator::genGMMIndices(int size, unsigned lod, unsigned flag)
  {
    unsigned step = static_cast<unsigned>(pow(2, lod));
    auto getIndex = [size](int x, int y) {
      return y * (size + 1) + x;
    };
    auto genIndices = [size, getIndex, step](const glm::uvec2& start_off, const glm::uvec2& end_off) {
      std::vector<unsigned> indices;
      for (int y = start_off.y; y + step <= size - end_off.y; y += step) {
        for (int x = start_off.x; x + step <= size - end_off.x; x += step) {
          indices.push_back(getIndex(x, y));
          indices.push_back(getIndex(x, y + step));
          indices.push_back(getIndex(x + step, y));
          indices.push_back(getIndex(x + step, y));
          indices.push_back(getIndex(x, y + step));
          indices.push_back(getIndex(x + step, y + step));
        }
      }
      return indices;
    };
    auto genSkirtHorizontal = [size, getIndex, step] (int y, int y_dir, bool swap) {
      std::vector<unsigned> ind;
      for (int x = 0; x + step * 2 <= size; x += step * 2) {
        std::vector<unsigned> indices;
        indices.push_back(getIndex(x, y));
        indices.push_back(getIndex(x, y + step * y_dir));
        indices.push_back(getIndex(x + step, y));
        indices.push_back(getIndex(x, y + step * y_dir));
        indices.push_back(getIndex(x + 2 * step, y + step * y_dir));
        indices.push_back(getIndex(x + step, y));
        indices.push_back(getIndex(x + step, y));
        indices.push_back(getIndex(x + 2 * step, y + step * y_dir));
        indices.push_back(getIndex(x + 2 * step, y));
        if (swap) {
          std::swap(indices[0], indices[1]);
          std::swap(indices[3], indices[4]);
          std::swap(indices[6], indices[7]);
        }
        ind.insert(ind.end(), indices.begin(), indices.end());
      }
      return ind;
    };
    auto genSkirtVertical = [size, getIndex, step](int x, int x_dir, bool swap) {
      std::vector<unsigned> ind;
      for (int y = 0; y + step * 2 <= size; y += step * 2) {
        std::vector<unsigned> indices;
        indices.push_back(getIndex(x, y));
        indices.push_back(getIndex(x + step * x_dir, y));
        indices.push_back(getIndex(x, y + step));
        indices.push_back(getIndex(x, y + step));
        indices.push_back(getIndex(x + step * x_dir, y));
        indices.push_back(getIndex(x + step * x_dir, y + step * 2));
        indices.push_back(getIndex(x, y + step));
        indices.push_back(getIndex(x + step * x_dir, y + step * 2));
        indices.push_back(getIndex(x, y + step * 2));
        if (swap) {
          std::swap(indices[0], indices[1]);
          std::swap(indices[3], indices[4]);
          std::swap(indices[6], indices[7]);
        }
        ind.insert(ind.end(), indices.begin(), indices.end());
      }
      return ind;
    };
    auto skirt_north = genSkirtHorizontal(size - step, 1, false);
    auto skirth_south = genSkirtHorizontal(step, -1, true);
    auto skirth_east = genSkirtVertical(size - step, 1, true);
    auto skirth_west = genSkirtVertical(step, -1, false);
    if (flag == SkirtFlag::North) {
      auto indices = genIndices(glm::uvec2(0), glm::uvec2(0, step));
      indices.insert(indices.end(), skirt_north.begin(), skirt_north.end());
      return indices;
    }
    else if (flag == SkirtFlag::South) {
      auto indices = genIndices(glm::uvec2(0, step), glm::uvec2(0));
      indices.insert(indices.end(), skirth_south.begin(), skirth_south.end());
      return indices;
    }
    else if (flag == SkirtFlag::East) {
      auto indices = genIndices(glm::uvec2(0), glm::uvec2(step, 0));
      indices.insert(indices.end(), skirth_east.begin(), skirth_east.end());
      return indices;
    }
    else if (flag == SkirtFlag::West) {
      auto indices = genIndices(glm::uvec2(step, 0), glm::uvec2(0));
      indices.insert(indices.end(), skirth_west.begin(), skirth_west.end());
      return indices;
    }
    else if (flag == (SkirtFlag::North | SkirtFlag::East)) {
      auto indices = genIndices(glm::uvec2(0), glm::uvec2(step));
      indices.insert(indices.end(), skirt_north.begin(), skirt_north.end() - 3);
      indices.insert(indices.end(), skirth_east.begin(), skirth_east.end() - 3);
      return indices;
    }
    else if (flag == (SkirtFlag::East | SkirtFlag::South)) {
      auto indices = genIndices(glm::uvec2(0, step), glm::uvec2(step, 0));
      indices.insert(indices.end(), skirth_east.begin() + 3, skirth_east.end());
      indices.insert(indices.end(), skirth_south.begin(), skirth_south.end() - 3);
      return indices;
    }
    else if (flag == (SkirtFlag::South | SkirtFlag::West)) { 
      auto indices = genIndices(glm::uvec2(step), glm::uvec2(0));
      indices.insert(indices.end(), skirth_south.begin() + 3, skirth_south.end());
      indices.insert(indices.end(), skirth_west.begin() + 3, skirth_west.end());
      return indices;
    }
    else if (flag == (SkirtFlag::West | SkirtFlag::North)) {
      auto indices = genIndices(glm::uvec2(step, 0), glm::uvec2(0, step));
      indices.insert(indices.end(), skirth_west.begin(), skirth_west.end()- 3);
      indices.insert(indices.end(), skirt_north.begin() + 3, skirt_north.end());
      return indices;
    }
    else if (flag == (SkirtFlag::North | SkirtFlag::South)) {
      auto indices = genIndices(glm::uvec2(0, step), glm::uvec2(0, step));
      indices.insert(indices.end(), skirt_north.begin(), skirt_north.end());
      indices.insert(indices.end(), skirth_south.begin(), skirth_south.end());
      return indices;
    }
    else if (flag == (SkirtFlag::East | SkirtFlag::West)) {
      auto indices = genIndices(glm::uvec2(step, 0), glm::uvec2(step, 0));
      indices.insert(indices.end(), skirth_west.begin(), skirth_west.end());
      indices.insert(indices.end(), skirth_east.begin(), skirth_east.end());
      return indices;
    }
    else if (flag == (SkirtFlag::North | SkirtFlag::East | SkirtFlag::South)) {
      auto indices = genIndices(glm::uvec2(0, step), glm::uvec2(step));
      indices.insert(indices.end(), skirt_north.begin(), skirt_north.end() - 3);
      indices.insert(indices.end(), skirth_east.begin() + 3, skirth_east.end() - 3);
      indices.insert(indices.end(), skirth_south.begin(), skirth_south.end() - 3);
      return indices;
    }
    else if (flag == (SkirtFlag::East | SkirtFlag::South | SkirtFlag::West)) {
      auto indices = genIndices(glm::uvec2(step), glm::uvec2(step, 0));
      indices.insert(indices.end(), skirth_east.begin() + 3, skirth_east.end());
      indices.insert(indices.end(), skirth_south.begin() + 3, skirth_south.end() - 3);
      indices.insert(indices.end(), skirth_west.begin() + 3, skirth_west.end());
      return indices;
    }
    else if (flag == (SkirtFlag::South | SkirtFlag::West | SkirtFlag::North)) {
      auto indices = genIndices(glm::uvec2(step), glm::uvec2(0, step));
      indices.insert(indices.end(), skirth_south.begin() + 3, skirth_south.end());
      indices.insert(indices.end(), skirth_west.begin() + 3, skirth_west.end() - 3);
      indices.insert(indices.end(), skirt_north.begin() + 3, skirt_north.end());
      return indices;
    }
    else if (flag == (SkirtFlag::West | SkirtFlag::North | SkirtFlag::East)) {
      auto indices = genIndices(glm::uvec2(step, 0), glm::uvec2(step));
      indices.insert(indices.end(), skirth_west.begin(), skirth_west.end() - 3);
      indices.insert(indices.end(), skirt_north.begin() + 3, skirt_north.end() - 3);
      indices.insert(indices.end(), skirth_east.begin(), skirth_east.end() - 3);
      return indices;
    }
    else if (flag == (SkirtFlag::South | SkirtFlag::West | SkirtFlag::North | SkirtFlag::East)) {
      auto indices = genIndices(glm::uvec2(step), glm::uvec2(step));
      indices.insert(indices.end(), skirth_south.begin() + 3, skirth_south.end() - 3);
      indices.insert(indices.end(), skirth_west.begin() + 3, skirth_west.end() - 3);
      indices.insert(indices.end(), skirt_north.begin() + 3, skirt_north.end() - 3);
      indices.insert(indices.end(), skirth_east.begin() + 3, skirth_east.end() - 3);
      return indices;
    }
    return genIndices(glm::uvec2(0), glm::uvec2(0));
  }
}