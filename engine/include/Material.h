#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <math/FlyMath.h>

namespace fly
{
  class Material
  {
  public:
    Material() = default;
    Material(const Vec3f& diffuse_color, float specular_exponent = 64.f, const std::string& diffuse_path = "", const std::string& normal_path = "", const std::string& opactiy_path = "");
    std::string& getDiffusePath();
    std::string& getNormalPath();
    std::string& getOpacityPath();
    float getSpecularExponent() const;
    const Vec3f& getDiffuseColor() const;
    void setDiffuseColor(const Vec3f& diffuse_color);
    void setSpecularExponent(float specular);
    void setDiffusePath(const std::string& diffuse_path);
    void setNormalPath(const std::string& normal_path);
    void setOpacityPath(const std::string& opacity_path);
    bool hasWindX() const;
    void setHasWindX(bool has_wind, float strength, float frequ);
    bool hasWindZ() const;
    void setHasWindZ(bool has_wind, float strength, float frequ);
    float getWindStrength() const;
    float getWindFrequency() const;
    void setIsReflective(bool reflective);
    bool isReflective() const;

  private:
    Vec3f _diffuseColor;
    // Texture paths
    std::string _diffusePath;
    std::string _normalPath;
    std::string _opacityPath;
    float _specularExponent;
    bool _hasWindX = false;
    bool _hasWindZ = false;
    float _windStrength;
    float _windFrequency;
    bool _isReflective = false;
  };
}

#endif // !MATERIAL_H
