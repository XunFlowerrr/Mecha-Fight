#include "ThrusterParticleSystem.h"

#include <algorithm>

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

namespace mecha
{

  void ThrusterParticleSystem::SetParticles(std::vector<ThrusterParticle> *particles)
  {
    source_ = particles;
  }

  void ThrusterParticleSystem::SetUpdateParams(const UpdateParams &params)
  {
    updateParams_ = params;
  }

  void ThrusterParticleSystem::Update(const UpdateContext &ctx)
  {
    if (!source_)
    {
      return;
    }

    const float dt = ctx.deltaTime;
    if (dt <= 0.0f)
    {
      return;
    }

    const float gravity = updateParams_.gravity;
    const float drag = glm::max(0.0f, updateParams_.drag);
    const float turbulenceStrength = updateParams_.turbulenceStrength;
    const float turbulenceFrequency = updateParams_.turbulenceFrequency;
    const float upwardDrift = updateParams_.upwardDrift;

    for (auto &p : *source_)
    {
      const float lifeRatio = glm::clamp(p.life / std::max(0.001f, p.maxLife), 0.0f, 1.0f);
      const float age = 1.0f - lifeRatio;

      glm::vec3 swirlAxis = glm::vec3(-p.vel.z, 0.0f, p.vel.x);
      if (glm::dot(swirlAxis, swirlAxis) < 0.0001f)
      {
        swirlAxis = glm::vec3(1.0f, 0.0f, 0.0f);
      }
      else
      {
        swirlAxis = glm::normalize(swirlAxis);
      }

      const float wave = glm::sin((age * turbulenceFrequency + p.seed * 6.2831853f) * 2.3f);
      glm::vec3 turbulence = swirlAxis * wave * turbulenceStrength;
      turbulence.y += upwardDrift * (0.3f + age * 0.7f);

      p.vel += turbulence * dt;
      p.vel.y -= gravity * dt;
      const float dragFactor = 1.0f / (1.0f + drag * dt);
      p.vel *= dragFactor;

      p.radiusScale = glm::mix(p.radiusScale, 1.25f, dt * 0.85f);
      p.intensity = glm::mix(p.intensity, 0.6f, dt * 0.7f);

      p.pos += p.vel * dt;
      p.life -= dt;
    }

    source_->erase(std::remove_if(source_->begin(), source_->end(), [](const ThrusterParticle &p)
                                  { return p.life <= 0.0f; }),
                   source_->end());
  }

  void ThrusterParticleSystem::Render(const RenderContext &ctx)
  {
    particles_.clear();
    if (source_)
    {
      for (const auto &src : *source_)
      {
        Particle particle;
        particle.position = src.pos;
        particle.velocity = src.vel;
        particle.life = src.life;
        particle.maxLife = src.maxLife;
        particle.intensity = src.intensity;
        particle.radiusScale = src.radiusScale;
        particle.seed = src.seed;
        particles_.push_back(particle);
      }
    }
    ParticleSystemBase::Render(ctx);
  }

  glm::vec4 ThrusterParticleSystem::ParticleColor(const Particle &p, const RenderParams &) const
  {
    const float lifeRatio = glm::clamp(p.life / std::max(0.001f, p.maxLife), 0.0f, 1.0f);
    const float age = 1.0f - lifeRatio;

    glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.95f, 0.82f), glm::vec3(1.0f, 0.7f, 0.25f), glm::smoothstep(0.0f, 0.35f, age));
    color = glm::mix(color, glm::vec3(1.0f, 0.35f, 0.05f), glm::smoothstep(0.2f, 0.7f, age));
    color = glm::mix(color, glm::vec3(0.18f, 0.18f, 0.18f), glm::smoothstep(0.65f, 1.0f, age));

    const float flicker = 0.85f + 0.15f * glm::sin((p.seed + age) * 18.8495559f);
    const float intensity = glm::clamp(p.intensity * flicker * 1.05f, 0.0f, 2.0f);
    color *= intensity;

    float alpha = glm::mix(0.95f, 0.0f, glm::smoothstep(0.15f, 1.0f, age));
    alpha *= glm::clamp(intensity, 0.0f, 1.0f);
    return glm::vec4(color, alpha);
  }

  float ThrusterParticleSystem::ParticleRadius(const Particle &p) const
  {
    const float lifeRatio = glm::clamp(p.life / std::max(0.001f, p.maxLife), 0.0f, 1.0f);
    const float expansion = 1.0f - lifeRatio;
    const float baseRadius = 0.035f + expansion * 0.18f;
    const float velocityStretch = glm::clamp(glm::length(p.velocity) / 22.0f, 0.6f, 1.6f);
    return baseRadius * p.radiusScale * velocityStretch;
  }

} // namespace mecha
