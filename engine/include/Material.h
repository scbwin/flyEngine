#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <math/FlyMath.h>
#include <vector>
#include <map>

namespace fly
{
  class Material
  {
  public:
    enum class TextureKey
    {
      ALPHA, ALBEDO, HEIGHT, NORMAL
    };
    Material() = default;
    float getSpecularExponent() const;
    const Vec3f& getDiffuseColor() const;
    void setDiffuseColor(const Vec3f& diffuse_color);
    void setSpecularExponent(float specular);
    void setTexturePath(TextureKey key, const std::string& path);
    const std::map<TextureKey, std::string>& getTexturePaths() const;
    const std::string& getTexturePath(TextureKey key) const;
    bool hasTexture(TextureKey key) const;
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
    void setDiffuseColors(const std::vector<Vec4f>& colors);
    const std::vector<Vec4f>& getDiffuseColors() const;

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
    std::map<TextureKey, std::string> _texturePaths;
    bool _isReflective = false;
    std::vector<Vec4f> _diffuseColors;
  };
}

#endif // !MATERIAL_H
