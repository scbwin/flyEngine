#ifndef IIMPORTER_H
#define IIMPORTER_H

#include <memory>
#include <string>

namespace fly
{
  class Model;

  class IImporter
  {
  public:
    IImporter() = default;
    virtual ~IImporter() = default;
    virtual std::shared_ptr<Model> loadModel(const std::string& path) = 0;
  };
}

#endif // !IIMPORTER_H
