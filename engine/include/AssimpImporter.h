#ifndef ASSIMPIMPORTER_H
#define ASSIMPIMPORTER_H

#include <IImporter.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace fly
{
  class Mesh;
  class Material;

  class AssimpImporter : public IImporter
  {
  public:
    AssimpImporter();
    virtual ~AssimpImporter() = default;
    virtual std::shared_ptr<Model> loadModel(const std::string& path) override;

  private:
    std::shared_ptr<Mesh> processMesh(aiMesh* mesh);
    std::shared_ptr<Material> processMaterial(aiMaterial* material, const std::string& path);
  };
}

#endif
