#define GLM_ENABLE_EXPERIMENTAL
#include <Terrain.h>
#include <iostream>
#include <Model.h>
#include <map>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_access.hpp>

#define USE_LOD_DISTANCE 1

namespace fly
{
  Terrain::Terrain(int tile_size, const WindParams& wind_params, float wind_strength, float grass_height, const glm::vec3& grass_color, const glm::vec3& terrain_col) :
    _tileSize(tile_size),
    _grassColor(grass_color),
    _terrainColor(terrain_col),
    _grassHeight(grass_height),
    _windParams(wind_params)
  {
  }

  std::string & Terrain::getDetailsNormalMap()
  {
    return _detailsNormalMap;
  }

  float Terrain::getHeight(int x, int z)
  {
    x = glm::clamp(x, 0, _heightMap.cols - 1);
    z = glm::clamp(z, 0, _heightMap.rows - 1);
    return _heightMap.at<float>(z, x);
  }
  cv::Mat& Terrain::getSplatMap()
  {
    return _splatMap;
  }

  cv::Mat & Terrain::getHeightMap()
  {
    return _heightMap;
  }

  int Terrain::getTileSize()
  {
    return _tileSize;
  }
  void Terrain::getTilesForRendering(std::map<int, std::map<int, int>>& lods, const glm::vec3 & cam_pos_model_space, const glm::mat4& mvp)
  {
    for (auto& e1 : _tiles) {
      for (auto& e2 : e1.second) {
        const auto& tile = e2.second;
          int lod = glm::distance(glm::vec2(cam_pos_model_space.x, cam_pos_model_space.z), tile.center()) / _tileSize;
          lod = glm::min(lod, _maxLOD);
          lods[tile._pos.x][tile._pos.y] = lod;
      }
    }

/*#if !USE_LOD_DISTANCE
    glm::ivec2 nearest_tile = glm::ivec2(cam_pos_ms.x, cam_pos_ms.z);
    nearest_tile -= glm::ivec2(nearest_tile.x % _tileSize, nearest_tile.y % _tileSize);
#endif
    for (int x = 0; x < _maxIndex; x += _tileSize) {
      for (int z = 0; z < _maxIndex; z += _tileSize) {
#if USE_LOD_DISTANCE
        int lod = glm::distance(glm::vec2(cam_pos_ms.x, cam_pos_ms.z), glm::vec2(x, z) + _tileSize * 0.5f) / _tileSize;
#else
        glm::ivec2 dist = abs(glm::ivec2(x, z) - nearest_tile) / _tileSize;
        int lod = glm::max(dist.x, dist.y);
#endif
        lod = glm::min(lod, _maxLOD);
        lods[x][z] = lod;
      }
    }*/
  }

  void Terrain::addTree(const glm::mat4 & transform, float scale)
  {
    _rootNode->addTree(transform, scale);
  }

  void Terrain::addCloudBillboard(const glm::vec3 & pos, float scale)
  {
    _rootNode->addCloudBillboard(pos, scale);
  }

  std::vector<glm::vec2> Terrain::getTileVertices()
  {
    std::vector<glm::vec2> vertices;
    for (int x = 0; x <= _tileSize; x++) {
      for (int z = 0; z <= _tileSize; z++) {
        _indexMap[x][z] = vertices.size();
        vertices.push_back(glm::vec2(x, z));
      }
    }
    return vertices;
  }
  std::vector<unsigned> Terrain::getTileIndices(int lod)
  {
    return getIndices(lod, glm::uvec2(0), glm::uvec2(0));
  }
  std::vector<unsigned> Terrain::getTileIndicesNorth(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 0), glm::uvec2(0, 1));
    auto skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesSouth(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 1), glm::uvec2(0, 0));
    auto skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesWest(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 0), glm::uvec2(0, 0));
    auto skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesEast(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 0), glm::uvec2(1, 0));
    auto skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesNorthSouth(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 1), glm::uvec2(0, 1));
    auto skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesEastWest(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 0), glm::uvec2(1, 0));
    auto skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesNorthEast(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 0), glm::uvec2(1, 1));
    auto skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesNorthWest(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 0), glm::uvec2(0, 1));
    auto skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesSouthWest(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 1), glm::uvec2(0, 0));
    auto skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesSouthEast(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 1), glm::uvec2(1, 0));
    auto skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesNorthEastSouth(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(0, 1), glm::uvec2(1, 1));
    auto skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    return indices;
  }
  std::vector<unsigned> Terrain::gettileIndicesEastSouthWest(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 1), glm::uvec2(1, 0));
    auto skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesSouthWestNorth(int lod)
  {

    auto indices = getIndices(lod, glm::uvec2(1, 1), glm::uvec2(0, 1));
    auto skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end());
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesWestNorthEast(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 0), glm::uvec2(1, 1));
    auto skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin(), skirt.end() - 3);
    return indices;
  }
  std::vector<unsigned> Terrain::getTileIndicesNorthWestSouthEast(int lod)
  {
    auto indices = getIndices(lod, glm::uvec2(1, 1), glm::uvec2(1, 1));
    auto skirt = skirtNorth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtWest(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtSouth(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    skirt = skirtEast(lod);
    indices.insert(indices.end(), skirt.begin() + 3, skirt.end() - 3);
    return indices;
  }
  int Terrain::getMaxLOD()
  {
    return _maxLOD;
  }

  void Terrain::setHeightMap(const cv::Mat & height_map)
  {
    _heightMap = height_map;
    _rootNode = std::unique_ptr<TreeNode>(new TreeNode(glm::ivec2(0), height_map.cols));
    buildQuadtree(_rootNode.get());
  }

  void Terrain::setSplatMapImage(const cv::Mat& mat)
  {
    _splatMap = mat;
  }

  std::vector<unsigned> Terrain::getPointIndices(int lod)
  {
    std::vector<unsigned> indices;
    auto step = stepFromLOD(lod);
    for (int x = 0; x <= _tileSize; x += step) {
      for (int z = 0; z <= _tileSize; z += step) {
        indices.push_back(_indexMap[x][z]);
      }
    }
    return indices;
  }

  glm::vec3 Terrain::getGrassColor()
  {
    return _grassColor;
  }

  glm::vec3 Terrain::getTerrainColor()
  {
    return _terrainColor;
  }

  void Terrain::setTreeModelLod0(const std::shared_ptr<Model>& model)
  {
    _treeModelLod0 = model;
  }

  void Terrain::setTreeModelLod1(const std::shared_ptr<Model>& model)
  {
    _treeModelLod1 = model;
  }

  void Terrain::setLeavesModel(const std::shared_ptr<Model>& model)
  {
    _leavesModel = model;
  }

  std::shared_ptr<Model> Terrain::getLeavesModel()
  {
    return _leavesModel;
  }

  std::shared_ptr<Model> Terrain::getTreeModelLod0()
  {
    return _treeModelLod0;
  }

  std::shared_ptr<Model> Terrain::getTreeModelLod1()
  {
    return _treeModelLod1;
  }

  void Terrain::getAllNodes(std::vector<TreeNode*>& nodes)
  {
    getAllNodes(_rootNode.get(), nodes);
  }

  void Terrain::getTreeNodesForRendering(const glm::vec3 & cam_pos_model_space, std::vector<TreeNode*>& nodes, const Mat4f& mvp, DirectionalLight* dl, const Mat4f& light_mvp)
  {
    getTreeNodesForRendering(_rootNode.get(), cam_pos_model_space, nodes, mvp, dl, light_mvp);
  }

  void Terrain::generateTiles()
  {
    for (int x = 0; x < _heightMap.cols; x += _tileSize) {
      for (int z = 0; z < _heightMap.rows; z += _tileSize) {
        _tiles[x].insert(std::make_pair(z, Tile(glm::ivec2(x, z), this)));
      }
    }
  }

  float Terrain::getGrassHeight()
  {
    return _grassHeight;
  }

  Terrain::WindParams & Terrain::getWindParams()
  {
    return _windParams;
  }

  void Terrain::build()
  {
    std::vector<TreeNode*> nodes;
    getAllNodes(nodes);

    for (auto& n : nodes) {
      n->_maxHeight = std::numeric_limits<float>::lowest();
      n->_minHeight = std::numeric_limits<float>::max();
      for (int x = n->_pos.x; x <= n->_pos.x + n->_size; x++) {
        for (int y = n->_pos.y; y <= n->_pos.y + n->_size; y++) {
          n->_maxHeight = std::max(n->_maxHeight, getHeight(x, y));
          n->_minHeight = std::min(n->_minHeight, getHeight(x, y));
        }
      }

      Vec3f bb_min({ static_cast<float>(n->_pos.x),static_cast<float>(n->_minHeight), static_cast<float>(n->_pos.y) });
      Vec3f bb_max({ static_cast<float>(n->_pos.x + n->_size), static_cast<float>(n->_maxHeight), static_cast<float>(n->_pos.y + n->_size) });

      n->_aabb = new AABB(bb_min, bb_max);
    }
  }

  void Terrain::getAllNodes(TreeNode * node, std::vector<TreeNode*>& nodes)
  {
    if (node) {
      nodes.push_back(node);
      getAllNodes(node->_southWest, nodes);
      getAllNodes(node->_southEast, nodes);
      getAllNodes(node->_northWest, nodes);
      getAllNodes(node->_northEast, nodes);
    }
  }

  void Terrain::getTreeNodesForRendering(TreeNode * node, const glm::vec3 & cam_pos_model_space, std::vector<TreeNode*>& nodes, const glm::mat4& mvp, DirectionalLight* dl, const Mat4f& light_mvp)
  {
    float dist = glm::distance(cam_pos_model_space, node->center());
    float error = node->_size / dist;
    if (error > _errorThreshold && node->_southWest) {
      getTreeNodesForRendering(node->_southWest, cam_pos_model_space, nodes, mvp, dl, light_mvp);
      getTreeNodesForRendering(node->_southEast, cam_pos_model_space, nodes, mvp, dl, light_mvp);
      getTreeNodesForRendering(node->_northWest, cam_pos_model_space, nodes, mvp, dl, light_mvp);
      getTreeNodesForRendering(node->_northEast, cam_pos_model_space, nodes, mvp, dl, light_mvp);
    }
    else {
      if (!dl) {
        if ((node->_transforms.size() || node->_cloudBillboardPositionsAndScales.size()) && node->_aabb->intersectsFrustum<false>(mvp)) {
          node->_lod = (node->_size / _minNodeSize) - 1;
          nodes.push_back(node);
        }
      }
      else {
        if ((node->_transforms.size() || node->_cloudBillboardPositionsAndScales.size()) && node->_aabb->intersectsFrustum<false>(light_mvp)) {
          node->_lod = (node->_size / _minNodeSize) - 1;
          nodes.push_back(node);
        }
      }
    }
  }

  std::vector<unsigned int> Terrain::getIndices(int lod, const glm::uvec2& low_offs, const glm::uvec2& up_offs)
  {
    auto step = stepFromLOD(lod);
    std::vector<unsigned> indices;
    glm::ivec2 from = glm::ivec2(low_offs) * step;
    glm::ivec2 to = _tileSize - glm::ivec2(up_offs) * step;
    for (int x = from.x; x + step <= to.x; x += step) {
      for (int z = from.y; z + step <= to.y; z += step) {
        indices.push_back(_indexMap[x][z]);
        indices.push_back(_indexMap[x][z + step]);
        indices.push_back(_indexMap[x + step][z]);
        indices.push_back(_indexMap[x + step][z + step]);
        indices.push_back(_indexMap[x + step][z]);
        indices.push_back(_indexMap[x][z + step]);
      }
    }
    return indices;
  }
  std::vector<unsigned> Terrain::skirtNorth(int lod)
  {
    auto step = stepFromLOD(lod);
    std::vector<unsigned> indices;
    for (int x = 0; x + step * 2 <= _tileSize; x += step * 2) {
      int z = _tileSize - step;
      indices.push_back(_indexMap[x][z]);
      indices.push_back(_indexMap[x][z + step]);
      indices.push_back(_indexMap[x + step][z]);

      indices.push_back(_indexMap[x][z + step]);
      indices.push_back(_indexMap[x + step * 2][z + step]);
      indices.push_back(_indexMap[x + step][z]);

      indices.push_back(_indexMap[x + step][z]);
      indices.push_back(_indexMap[x + step * 2][z + step]);
      indices.push_back(_indexMap[x + step * 2][z]);
    }
    return indices;
  }
  std::vector<unsigned> Terrain::skirtEast(int lod)
  {
    auto step = stepFromLOD(lod);
    std::vector<unsigned> indices;
    for (int z = 0; z + step * 2 <= _tileSize; z += step * 2) {
      int x = _tileSize - step;
      indices.push_back(_indexMap[x][z]);
      indices.push_back(_indexMap[x][step + z]);
      indices.push_back(_indexMap[x + step][z]);

      indices.push_back(_indexMap[x + step][z]);
      indices.push_back(_indexMap[x][z + step]);
      indices.push_back(_indexMap[x + step][z + step * 2]);

      indices.push_back(_indexMap[x][z + step]);
      indices.push_back(_indexMap[x][z + step * 2]);
      indices.push_back(_indexMap[x + step][z + step * 2]);
    }
    return indices;
  }
  std::vector<unsigned> Terrain::skirtWest(int lod)
  {
    auto step = stepFromLOD(lod); {}
    std::vector<unsigned> indices;
    for (int z = 0; z + step * 2 <= _tileSize; z += step * 2) {
      int x = 0;
      indices.push_back(_indexMap[x][z]);
      indices.push_back(_indexMap[x + step][z + step]);
      indices.push_back(_indexMap[x + step][z]);

      indices.push_back(_indexMap[x][z]);
      indices.push_back(_indexMap[x][z + step * 2]);
      indices.push_back(_indexMap[x + step][z + step]);

      indices.push_back(_indexMap[x][z + step * 2]);
      indices.push_back(_indexMap[x + step][z + step * 2]);
      indices.push_back(_indexMap[x + step][z + step]);
    }
    return indices;
  }
  std::vector<unsigned> Terrain::skirtSouth(int lod)
  {
    auto step = stepFromLOD(lod);
    std::vector<unsigned> indices;
    for (int x = 0; x + step * 2 <= _tileSize; x += step * 2) {
      int z = step;
      indices.push_back(_indexMap[x][z]);
      indices.push_back(_indexMap[x + step][z]);
      indices.push_back(_indexMap[x][z - step]);

      indices.push_back(_indexMap[x][z - step]);
      indices.push_back(_indexMap[x + step][z]);
      indices.push_back(_indexMap[x + step * 2][z - step]);

      indices.push_back(_indexMap[x + step][z]);
      indices.push_back(_indexMap[x + step * 2][z]);
      indices.push_back(_indexMap[x + step * 2][z - step]);
    }
    return indices;
  }
  int Terrain::stepFromLOD(int lod)
  {
    return pow(2, lod);
  }
  void Terrain::buildQuadtree(TreeNode * node)
  {
    int new_size = node->_size / 2;
    if (new_size >= _minNodeSize) {
      node->_southWest = new TreeNode(node->_pos, new_size);
      node->_southEast = new TreeNode(node->_pos + glm::ivec2(new_size, 0), new_size);
      node->_northWest = new TreeNode(node->_pos + glm::ivec2(0, new_size), new_size);
      node->_northEast = new TreeNode(node->_pos + new_size, new_size);
      buildQuadtree(node->_southWest);
      buildQuadtree(node->_southEast);
      buildQuadtree(node->_northWest);
      buildQuadtree(node->_northEast);
    }
  }

  Terrain::TreeNode::TreeNode(const glm::ivec2 & pos, int size) : _pos(pos), _size(size)
  {
  /*  for (unsigned int i = 0; i < all_transforms.size(); i++) {
      auto& t = all_transforms[i];
      auto pos_terrain = t * glm::vec4(0.f, 0.f, 0.f, 1.f);
      if (pos_terrain.x >= pos.x && pos_terrain.z >= pos.y && pos_terrain.x < pos.x + size && pos_terrain.z < pos.y + size) {
        _transforms.push_back(t);
        _scales.push_back(all_scales[i]);
      }
    }

    _maxHeight = std::numeric_limits<float>::lowest();
    _minHeight = std::numeric_limits<float>::max();
    for (int x = pos.x; x <= pos.x + size; x++) {
      for (int y = pos.y; y <= pos.y + size; y++) {
        _maxHeight = std::max(_maxHeight, g->getHeight(x, y));
        _minHeight = std::min(_minHeight, g->getHeight(x, y));
      }
    }

    glm::vec3 bb_min(pos.x, _minHeight, pos.y);
    glm::vec3 bb_max(pos.x + size, _maxHeight, pos.y + size);

    _aabb = new AABB(bb_min, bb_max);*/
  }
  void Terrain::TreeNode::addTree(const glm::mat4 & transform, float scale)
  {
    auto pos_terrain = glm::column(transform, 3);
    if (pos_terrain.x >= _pos.x && pos_terrain.z >= _pos.y && pos_terrain.x < _pos.x + _size && pos_terrain.z < _pos.y + _size) {
      _transforms.push_back(transform);
      _scales.push_back(scale);

      if (_southWest) {
        _southWest->addTree(transform, scale);
        _southEast->addTree(transform, scale);
        _northWest->addTree(transform, scale);
        _northEast->addTree(transform, scale);
      }
    }
  }
  void Terrain::TreeNode::addCloudBillboard(const glm::vec3 & pos, float scale)
  {
    if (pos.x >= _pos.x && pos.z >= _pos.y && pos.x < _pos.x + _size && pos.z < _pos.y + _size) {
      _cloudBillboardPositionsAndScales.push_back(glm::vec4(pos, scale));
      if (_southWest) {
        _southWest->addCloudBillboard(pos, scale);
        _southEast->addCloudBillboard(pos, scale);
        _northWest->addCloudBillboard(pos, scale);
        _northEast->addCloudBillboard(pos, scale);
      }
    }
  }
  Terrain::TreeNode::~TreeNode()
  {
    if (_southEast) {
      delete _southEast;
      delete _southWest;
      delete _northEast;
      delete _northWest;
    }
    delete _aabb;
  }
  glm::vec3 Terrain::TreeNode::center() const
  {
    auto grid_center = glm::vec2(_pos) + _size * 0.5f;
    return glm::vec3(grid_center.x, (_minHeight + _maxHeight) * 0.5f, grid_center.y);
  }

  Terrain::Tile::Tile(const glm::ivec2 & pos, Terrain * g) : _pos(pos), _size(g->_tileSize)
  {
    float min_height = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::lowest();
    for (int x = pos.x; x <= pos.x + g->_tileSize; x++) {
      for (int z = pos.y; z <= pos.y + g->_tileSize; z++) {
        float height = g->getHeight(x, z);
        min_height = std::min(min_height, height);
        max_height = std::max(max_height, height);
      }
    }

    glm::vec3 bb_min(pos.x, min_height, pos.y);
    glm::vec3 bb_max(pos.x + g->_tileSize + 1, max_height, pos.y + g->_tileSize + 1);

    _aabb[0] = glm::vec3(bb_min.x, bb_min.y, bb_min.z);
    _aabb[1] = glm::vec3(bb_max.x, bb_min.y, bb_min.z);
    _aabb[2] = glm::vec3(bb_min.x, bb_max.y, bb_min.z);
    _aabb[3] = glm::vec3(bb_min.x, bb_min.y, bb_max.z);
    _aabb[4] = glm::vec3(bb_max.x, bb_max.y, bb_min.z);
    _aabb[5] = glm::vec3(bb_min.x, bb_max.y, bb_max.z);
    _aabb[6] = glm::vec3(bb_max.x, bb_min.y, bb_max.z);
    _aabb[7] = glm::vec3(bb_max.x, bb_max.y, bb_max.z);
  }
  glm::vec2 Terrain::Tile::center() const
  {
    return glm::vec2(_pos) + _size * 0.5f;
  }
  bool Terrain::Tile::isVisible(const glm::mat4& mvp) const
  {
    bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_near = false, inside_far = false;
    for (unsigned int i = 0; i < 8; i++) {
      auto p = mvp * glm::vec4(_aabb[i], 1.f);
      inside_left = inside_left || p.x >= -p.w;
      inside_right = inside_right || p.x <= p.w;
      inside_bottom = inside_bottom || p.y >= -p.w;
      inside_top = inside_top || p.y <= p.w;
      inside_near = inside_near || p.z >= -p.w;
      inside_far = inside_far || p.z <= p.w;
    }
    bool inside = inside_left && inside_right && inside_bottom && inside_top && inside_near && inside_far;
    return inside;
  }
  Terrain::WindParams::WindParams(const glm::vec2 & dir, float strength, float frequency) : _dir(dir), _strength(strength), _frequency(frequency)
  {
  }
}