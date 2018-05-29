#include <LevelOfDetail.h>
#include <Model.h>
#include <Mesh.h>

namespace fly
{
  std::vector<std::shared_ptr<Model>> LevelOfDetail::generateLODsWithDetailCulling(const std::shared_ptr<Model>& model, unsigned lods)
  {
    std::vector<std::shared_ptr<Model>> ret = { model };

    auto getMaxSideLength = [](AABB const & aabb) {
      auto vec = aabb.getMax() - aabb.getMin();
      return std::max(vec[0], std::max(vec[1], vec[2]));
    };

    auto max_side_length_model = getMaxSideLength(model->getAABB());
    for (unsigned i = 1; i < lods; i++) {
      float ratio_thresh = static_cast<float>(i) / static_cast<float>(lods);
      std::vector<std::shared_ptr<Mesh>> meshes;
      for (const auto& mesh : model->getMeshes()) {
        float ratio = getMaxSideLength(mesh->getAABB()) / max_side_length_model;
        if (ratio >= ratio_thresh) {
          meshes.push_back(mesh);
        }
      }
      if (meshes.size()) {
        ret.push_back(std::make_shared<Model>(meshes, model->getMaterials()));
      }
      else {
        break;
      }
    }

    return ret;
  }
}