#include <physics/ParticleSystem.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <Windows.h>

namespace fly
{
  ParticleSystem::ParticleSystem(const ParticleSystemDesc& desc) : _desc(desc), _gen(_rd())
  {
  }
  void ParticleSystem::update(float time, float delta_time)
  {
    if (time >= _emit) {
      if (!_desc._circular) {
        _particles.push_front(emitParticle(time));
      }
      else {
        unsigned num_particles = 20;
        float delta = glm::two_pi<float>() / num_particles;
        float radians = 0.f;
        for (unsigned i = 0; i < num_particles; i++, radians += delta) {
          Particle p;
          p._birth = time;
          p._velocity = glm::vec3(0.f);
          p._position = glm::vec3(0.f);
          p._impulse = glm::vec3(cos(radians), 0.f, sin(radians)) * distance(_desc._initialVelocity, glm::vec3(0.f));
          _particles.push_front(p);
        }
      }
      _particles.resize((std::min)(static_cast<unsigned>(_particles.size()), _desc._maxParticles));
      _emit = time + _desc._emitInterval;
    }

    for (int i = static_cast<int>(_particles.size() - 1); i >= 0; i--) {
      auto& p = _particles[i];
      if (time >= p._birth && time <= p._birth + _desc._lifeTime) {
        auto acceleration = (p._impulse + _desc._gravity) / _desc._mass;
        p._velocity += acceleration * delta_time;
        p._position += p._velocity * delta_time;
        p._impulse *= 0.f;
        if (_desc._randomImpulses && random() < _desc._impulseProbability) {
          p._impulse = _desc._impulseStrength * (glm::vec3(randomScaleBiased(), randomScaleBiased(), randomScaleBiased()));
        }
        p._age = (time - p._birth) / _desc._lifeTime;
      }
      else if (time > p._birth + _desc._lifeTime) {
        _particles.erase(_particles.begin() + i);
      }
    }
  }
  void ParticleSystem::getParticleTransformations(std::vector<Mat4f>& transformations) const
  {
    float index = 0.f;
    assert(!transformations.size());
    transformations.reserve(_particles.size());
    for (const auto& p : _particles) {
      glm::vec3 y_vec = normalize(p._velocity);
      glm::vec3 x_vec = normalize(glm::vec3(y_vec.y, -y_vec.x, 0.f));
      glm::vec3 z_vec = cross(x_vec, y_vec);
      //float fade = 1.f - glm::smoothstep(_particles.size() * 0.75f, _particles.size() - 1.f, index);
     // fade *= glm::smoothstep(0.f, _particles.size() * 0.1f, index);
      float fade = glm::smoothstep(0.f, 0.2f, p._age) * (1.f - glm::smoothstep(0.8f, 1.f, p._age));
      glm::mat4 transform(glm::mat4(glm::vec4(x_vec, 0.f), glm::vec4(y_vec, 0.f), glm::vec4(z_vec, 0.f), glm::vec4(p._position, 1.f)) * glm::scale(glm::vec3(fade)));
      transformations.push_back(Mat4f(&transform[0][0]));
      index++;
    }
  }
  std::deque<Particle>& ParticleSystem::getParticles()
  {
    return _particles;
  }
  Particle ParticleSystem::emitParticle(float time)
  {
    return { glm::vec3(0.f), glm::vec3(0.f), _desc._initialVelocity + _desc._initialVelocityRandomScale * glm::vec3(randomScaleBiased(), randomScaleBiased(), randomScaleBiased()), 0.f, time };
  }
}