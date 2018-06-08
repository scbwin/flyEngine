#include <AssimpImporter.h>
#include <Model.h>
#include <Vertex.h>
#include <Mesh.h>

#define FLY_VEC2(vec) fly::Vec2f(vec.x, vec.y)
#define FLY_VEC3(vec) fly::Vec3f(vec.x, vec.y, vec.z)

namespace fly
{
  AssimpImporter::AssimpImporter() : IImporter()
  {
  }
  std::shared_ptr<Model> AssimpImporter::loadModel(const std::string & path)
  {
    Assimp::Importer importer;
    auto scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs);
    std::vector<std::shared_ptr<Mesh>> meshes(scene->mNumMeshes);
    std::vector<std::shared_ptr<Material>> materials(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
      materials[i] = processMaterial(scene->mMaterials[i], path);
    }
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
      meshes[i] = processMesh(scene->mMeshes[i], materials);
    }
    return std::make_shared<Model>(meshes, materials);
  }
  std::shared_ptr<Mesh> AssimpImporter::processMesh(aiMesh * mesh, const std::vector<std::shared_ptr<Material>>& materials)
  {
    std::vector<Vertex> vertices(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      vertices[i]._position = FLY_VEC3(mesh->mVertices[i]);
      vertices[i]._normal = CompressedNormal(FLY_VEC3(mesh->mNormals[i]));
      if (mesh->mTextureCoords[0] != nullptr) {
        vertices[i]._uv = FLY_VEC2(mesh->mTextureCoords[0][i]);
        vertices[i]._tangent = CompressedNormal(FLY_VEC3(mesh->mTangents[i]));
        vertices[i]._bitangent = CompressedNormal(FLY_VEC3(mesh->mBitangents[i]) * -1.f);
      }
    }
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
        indices.push_back(mesh->mFaces[i].mIndices[j]);
      }
    }
    auto m = std::make_shared<Mesh>(vertices, indices, mesh->mMaterialIndex);
    m->setMaterial(materials[mesh->mMaterialIndex]);
    return m;
  }
  std::shared_ptr<Material> AssimpImporter::processMaterial(aiMaterial * material, const std::string & path)
  {
#ifdef _WINDOWS
    char dir[_MAX_DIR];
    _splitpath(path.c_str(), nullptr, dir, nullptr, nullptr);
    std::string pre = dir;
#else
    // TODO linux implementation comes here
#endif
    auto mat = std::make_shared<Material>();
    if (material->GetTextureCount(aiTextureType::aiTextureType_DIFFUSE)) {
      aiString str;
      material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &str);
      mat->setTexturePath(Material::TextureKey::ALBEDO, pre + str.C_Str());
    }
    if (material->GetTextureCount(aiTextureType::aiTextureType_OPACITY)) {
      aiString str;
      material->GetTexture(aiTextureType::aiTextureType_OPACITY, 0, &str);
      mat->setTexturePath(Material::TextureKey::ALPHA, pre + str.C_Str());
    }
    if (material->GetTextureCount(aiTextureType::aiTextureType_HEIGHT)) {
      aiString str;
      material->GetTexture(aiTextureType::aiTextureType_HEIGHT, 0, &str);
      mat->setTexturePath(Material::TextureKey::NORMAL, pre + str.C_Str());
    }
    aiColor3D diff;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diff);
    float s;
    material->Get(AI_MATKEY_SHININESS, s);
    mat->setDiffuseColor(fly::Vec3f(diff.r, diff.g, diff.b));
    mat->setSpecularExponent(s);
    return mat;
  }
}