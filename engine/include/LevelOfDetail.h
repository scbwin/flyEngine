#ifndef LEVELOFDETAIL_H
#define LEVELOFDETAIL_H

#include <memory>
#include <vector>

namespace fly
{
  class Model;
  class LevelOfDetail
  {
  public:
    LevelOfDetail() = default;
    virtual ~LevelOfDetail() = default;
    std::vector<std::shared_ptr<Model>> generateLODsWithDetailCulling(const std::shared_ptr<Model>& model, unsigned lods = 3);
  private:
  };
}

#endif 
