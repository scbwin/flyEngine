#ifndef TERRAIN_H
#define TERRAIN_H

#include <opencv2/opencv.hpp>
#include <memory>
#include <glm/glm.hpp>
#include <Component.h>
#include <vector>
#include <map>
#include <array>
#include <AABB.h>

namespace fly
{
  class Model;
  class Mesh;

  class Terrain : public Component
  {
  public:
    struct WindParams
    {
      glm::vec2 _dir;
      float _strength;
      float _frequency;
      WindParams(const glm::vec2& dir, float strength, float frequency);
    };
    Terrain(int tile_size, const WindParams& wind_params = WindParams(glm::vec2(1.f), 2.5f, 0.05f), float wind_strength = 1.f, float grass_height = 0.23f, const glm::vec3& grass_color = glm::vec3(83.f, 124.f, 59.f) / 255.f, const glm::vec3& terrain_col = glm::vec3(94.f, 82.f, 66.f) / 255.f);
    std::string& getDetailsNormalMap();
    float getHeight(int x, int z);
    cv::Mat& getSplatMap();
    cv::Mat& getHeightMap();
    int getTileSize();
    void getTilesForRendering(std::map<int, std::map<int, int>>& lods, const glm::vec3& cam_pos_model_space, const glm::mat4& mvp);

    struct TreeNode
    {
      glm::ivec2 _pos;
      int _size;
      float _minHeight;
      float _maxHeight;
      std::vector<glm::mat4> _transforms;
      std::vector<float> _scales;
      std::vector<glm::vec4> _cloudBillboardPositionsAndScales;
      TreeNode(const glm::ivec2& pos, int size);
      void addTree(const glm::mat4& transform, float scale);
      void addCloudBillboard(const glm::vec3& pos, float scale);
      ~TreeNode();
      glm::vec3 center() const;
      TreeNode* _southWest = nullptr;
      TreeNode* _southEast = nullptr;
      TreeNode* _northWest = nullptr;
      TreeNode* _northEast = nullptr;
      int _lod = 0;
      AABB* _aabb;
    };

    void addTree(const glm::mat4& transform, float scale);
    void addCloudBillboard(const glm::vec3& pos, float scale);

    std::vector<glm::vec2> getTileVertices();
    std::vector<unsigned> getTileIndices(int lod);
    std::vector<unsigned> getTileIndicesNorth(int lod);
    std::vector<unsigned> getTileIndicesSouth(int lod);
    std::vector<unsigned> getTileIndicesWest(int lod);
    std::vector<unsigned> getTileIndicesEast(int lod);
    std::vector<unsigned> getTileIndicesNorthSouth(int lod);
    std::vector<unsigned> getTileIndicesEastWest(int lod);
    std::vector<unsigned> getTileIndicesNorthEast(int lod);
    std::vector<unsigned> getTileIndicesNorthWest(int lod);
    std::vector<unsigned> getTileIndicesSouthWest(int lod);
    std::vector<unsigned> getTileIndicesSouthEast(int lod);
    std::vector<unsigned> getTileIndicesNorthEastSouth(int lod);
    std::vector<unsigned> gettileIndicesEastSouthWest(int lod);
    std::vector<unsigned> getTileIndicesSouthWestNorth(int lod);
    std::vector<unsigned> getTileIndicesWestNorthEast(int lod);
    std::vector<unsigned> getTileIndicesNorthWestSouthEast(int lod);
    int getMaxLOD();

    void setHeightMap(const cv::Mat& height_map);
    void setSplatMapImage(const cv::Mat& mat);
    std::vector<unsigned> getPointIndices(int lod);
    glm::vec3 getGrassColor();
    glm::vec3 getTerrainColor();
    void setTreeModelLod0(const std::shared_ptr<Model>& model);
    void setTreeModelLod1(const std::shared_ptr<Model>& model);
    void setLeavesModel(const std::shared_ptr<Model>& model);
    std::shared_ptr<Model> getLeavesModel();
    std::shared_ptr<Model> getTreeModelLod0();
    std::shared_ptr<Model> getTreeModelLod1();
    void getAllNodes(std::vector<TreeNode*>& nodes);
    void getTreeNodesForRendering(const glm::vec3& cam_pos_model_space, std::vector<TreeNode*>& nodes, const glm::mat4& mvp);
    struct Tile
    {
      Tile(const glm::ivec2& pos, Terrain* g);
      glm::vec2 center() const;
      bool isVisible(const glm::mat4& mvp) const;
      glm::ivec2 _pos;
      int _size;
      std::array<glm::vec3, 8> _aabb;
    };
    void generateTiles();
    std::map<int, std::map<int, Tile>> _tiles;
    float getGrassHeight();
    WindParams& getWindParams();
    void build();

  private:
    glm::vec2 _min, _max;
    void getAllNodes(TreeNode* node, std::vector<TreeNode*>& nodes);
    void getTreeNodesForRendering(TreeNode* node, const glm::vec3& cam_pos_model_space, std::vector<TreeNode*>& nodes, const glm::mat4& mvp);
    std::map<int, std::map<int, unsigned>> _indexMap;

    int _tileSize;
    glm::ivec2 _maxTile = glm::ivec2(0, 0);

    int _maxLOD = 5;

    unsigned int _minNodeSize = 32;
    float _errorThreshold = 0.8f;

    WindParams _windParams;

    std::string _detailsNormalMap;

    glm::vec3 _grassColor;
    glm::vec3 _terrainColor;

    cv::Mat _heightMap;
    cv::Mat _splatMap;

    std::shared_ptr<Model> _treeModelLod0;
    std::shared_ptr<Model> _treeModelLod1;
    std::shared_ptr<Model> _leavesModel;

    std::unique_ptr<TreeNode> _rootNode;


    float _grassHeight;

    std::vector<unsigned int> getIndices(int lod, const glm::uvec2& low_offs, const glm::uvec2& up_offs);
    std::vector<unsigned> skirtNorth(int lod);
    std::vector<unsigned> skirtEast(int lod);
    std::vector<unsigned> skirtWest(int lod);
    std::vector<unsigned> skirtSouth(int lod);

    int stepFromLOD(int lod);
    void buildQuadtree(TreeNode* node);

  };
}

#endif // !GEOMIPMAP_H
