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
    std::string& getHeightPath();
    float getSpecularExponent() const;
    const Vec3f& getDiffuseColor() const;
    void setDiffuseColor(const Vec3f& diffuse_color);
    void setSpecularExponent(float specular);
    void setDiffusePath(const std::string& diffuse_path);
    void setNormalPath(const std::string& normal_path);
    void setOpacityPath(const std::string& opacity_path);
    void setHeightPath(const std::string& height_path);
    bool hasWindX() const;
    void setHasWindX(bool has_wind, float strength, float frequ);
    bool hasWindZ() const;
    void setHasWindZ(bool has_wind, float strength, float frequ);
    float getWindStrength() const;
    float getWindFrequency() const;
    void setIsReflective(bool reflective);
    bool isReflective() const;
    float getKa() const;
    float getKd() const;
    float getKs() const;
    void setKa(float ka);
    void setKd(float kd);
    void setKs(float ks);
    float getParallaxHeightScale() const;
    void setParallaxHeightScale(float height_scale);
    float getParallaxMinSteps() const;
    float getParallaxMaxSteps() const;
    void setParallaxMinSteps(float min_steps);
    void setParallaxMaxSteps(float max_steps);
    float getParallaxBinarySearchSteps() const;
    void setParallaxBinarySearchSteps(float steps);

  private:
    float _ka = 0.025f;
    float _kd = 1.f;
    float _ks = 1.f;
    float _specularExponent = 48.f;
    float _parallaxHeightScale = 0.04f;
    float _parallaxMinSteps = 1.f;
    float _parallaxMaxSteps = 2.f;
    float _parallaxBinarySearchSteps = 6.f;
    Vec3f _diffuseColor;
    std::string _diffusePath;
    std::string _normalPath;
    std::string _opacityPath;
    std::string _heightPath;
    bool _isReflective = false;

    // Deprecated
    bool _hasWindX = false;
    bool _hasWindZ = false;
    float _windStrength;
    float _windFrequency;
  };
}

#endif // !MATERIAL_H
