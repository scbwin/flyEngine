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
  std::string & Material::getHeightPath()
  {
    return _heightPath;
  }
  void Material::setSpecularExponent(float specular)
  {
    _specularExponent = std::max(1.f, specular);
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
  void Material::setHeightPath(const std::string & height_path)
  {
    _heightPath = height_path;
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
}