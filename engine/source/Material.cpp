#include <Material.h>

namespace fly
{
  Material::Material(const Vec3f& diffuse_color, float specular_exponent, const std::string & diffuse_path, 
    const std::string & normal_path, const std::string & opactiy_path) : _diffuseColor(diffuse_color), 
    _specularExponent(specular_exponent), _diffusePath(diffuse_path), _normalPath(normal_path), _opacityPath(opactiy_path)
  {
  }
  std::string& Material::getDiffusePath()
  {
    return _diffusePath;
  }
  std::string& Material::getNormalPath()
  {
    return _normalPath;
  }
  std::string& Material::getOpacityPath()
  {
    return _opacityPath;
  }
  void Material::setSpecularExponent(float specular)
  {
    _specularExponent = specular;
  }
  void Material::setDiffusePath(const std::string & diffuse_path)
  {
    _diffusePath = diffuse_path;
  }
  void Material::setNormalPath(const std::string & normal_path)
  {
    _normalPath = normal_path;
  }
  void Material::setOpacityPath(const std::string & opacity_path)
  {
    _opacityPath = opacity_path;
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
  bool Material::hasWindX() const
  {
    return _hasWindX;
  }
  void Material::setHasWindX(bool has_wind, float strength, float frequ)
  {
    _hasWindX = has_wind;
    _windStrength = strength;
    _windFrequency = frequ;
  }
  bool Material::hasWindZ() const
  {
    return _hasWindZ;
  }
  void Material::setHasWindZ(bool has_wind, float strength, float frequ)
  {
    _hasWindZ = has_wind;
    _windStrength = strength;
    _windFrequency = frequ;
  }
  float Material::getWindStrength() const
  {
    return _windStrength;
  }
  float Material::getWindFrequency() const
  {
    return _windFrequency;
  }
  void Material::setIsReflective(bool reflective)
  {
    _isReflective = reflective;
  }
  bool Material::isReflective() const
  {
    return _isReflective;
  }
}