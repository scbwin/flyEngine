#include <Material.h>

namespace fly
{
  void Material::setSpecularExponent(float specular)
  {
    _specularExponent = std::max(1.f, specular);
  }
  void Material::setTexturePath(Material::TextureKey key, const std::string & path)
  {
    _texturePaths[key] = path;
  }
  const std::map<Material::TextureKey, std::string>& Material::getTexturePaths() const
  {
    return _texturePaths;
  }
  const std::string & Material::getTexturePath(Material::TextureKey key) const
  {
    return _texturePaths.at(key);
  }
  bool Material::hasTexture(Material::TextureKey key) const
  {
    return _texturePaths.find(key) != _texturePaths.end();
  }
  float Material::getSpecularExponent() const
  {
    return _specularExponent;
  }
  const Vec3f& Material::getDiffuseColor() const
  {
    return _diffuseColor;
  }
  void Material::setDiffuseColor(const Vec3f & diffuse_color)
  {
    _diffuseColor = diffuse_color;
  }
  void Material::setIsReflective(bool reflective)
  {
    _isReflective = reflective;
  }
  bool Material::isReflective() const
  {
    return _isReflective;
  }
  float Material::getKa() const
  {
    return _ka;
  }
  float Material::getKd() const
  {
    return _kd;
  }
  float Material::getKs() const
  {
    return _ks;
  }
  void Material::setKa(float ka)
  {
    _ka = ka;
  }
  void Material::setKd(float kd)
  {
    _kd = kd;
  }
  void Material::setKs(float ks)
  {
    _ks = ks;
  }
  float Material::getParallaxHeightScale() const
  {
    return _parallaxHeightScale;
  }
  void Material::setParallaxHeightScale(float height_scale)
  {
    _parallaxHeightScale = height_scale;
  }
  float Material::getParallaxMinSteps() const
  {
    return _parallaxMinSteps;
  }
  float Material::getParallaxMaxSteps() const
  {
    return _parallaxMaxSteps;
  }
  void Material::setParallaxMinSteps(float min_steps)
  {
    _parallaxMinSteps = std::min(min_steps, _parallaxMaxSteps);
    _parallaxMinSteps = std::max(_parallaxMinSteps, 1.f);
  }
  void Material::setParallaxMaxSteps(float max_steps)
  {
    _parallaxMaxSteps = std::max(max_steps, _parallaxMinSteps);
  }
  float Material::getParallaxBinarySearchSteps() const
  {
    return _parallaxBinarySearchSteps;
  }
  void Material::setParallaxBinarySearchSteps(float steps)
  {
    _parallaxBinarySearchSteps = std::max(steps, 0.f);
  }
  void Material::setDiffuseColors(const std::vector<Vec4f>& colors)
  {
    _diffuseColors = colors;
  }
  const std::vector<Vec4f>& Material::getDiffuseColors() const
  {
    return _diffuseColors;
  }
}