#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include <Transform.h>
#include <Model.h>
#include <System.h>
#include <deque>
#include <random>
#include <math/FlyMath.h>

namespace fly
{
  struct Particle
  {
    glm::vec3 _position;
    glm::vec3 _velocity;
    glm::vec3 _impulse;
    float _age;
    float _birth;
  };

  class ParticleSystem : public Component
  {
  public:
    struct ParticleSystemDesc
    {
      glm::vec3 _gravity;
      float _mass;
      float _emitInterval;
      float _lifeTime;
      unsigned _maxParticles;
      glm::vec3 _initialVelocity;
      glm::vec3 _initialVelocityRandomScale;
      bool _randomImpulses;
      float _impulseStrength;
      float _impulseProbability;
      bool _circular;
    };
    ParticleSystem(const ParticleSystemDesc& desc);
    void update(float time, float delta_time);
    std::deque<Particle> _particles;
    void getParticleTransformations(std::vector<Mat4f>& transformations) const;
    std::deque<Particle>& getParticles();
  private:
    ParticleSystemDesc _desc;
    Particle emitParticle(float time);
  //  glm::vec3 _gravity = glm::vec3(0.f, 0.01f, 0.f);
  //  float _mass = 0.005f;
    float _emit = 0.f;
  //  float _particleLifeTime;
   // unsigned _maxParticles;
    std::random_device _rd;
    std::mt19937 _gen;
    std::uniform_real_distribution<float> _dist = std::uniform_real_distribution<float>(0.f, 1.f);
    inline float random()
    {
      return _dist(_gen);
    }
    inline float randomScaleBiased()
    {
      return _dist(_gen) * 2.f - 1.f;
    }
  };
}

#endif
