#include "DashParticleSystem.h"

#include <algorithm>

namespace mecha
{

  void DashParticleSystem::SetParticles(std::vector<DashParticle> *particles)
  {
    source_ = particles;
  }

  void DashParticleSystem::Update(const UpdateContext &ctx)
  {
    if (!source_)
    {
      return;
    }

    const float dt = ctx.deltaTime;

    for (auto &p : *source_)
    {
      p.pos += p.vel * dt;
      p.life -= dt;
      p.vel *= 0.95f;
    }

    source_->erase(std::remove_if(source_->begin(), source_->end(), [](const DashParticle &p)
                                  { return p.life <= 0.0f; }),
                   source_->end());
  }

  void DashParticleSystem::Render(const RenderContext &ctx)
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
        particle.intensity = 1.0f;
        particle.radiusScale = 1.0f;
        particle.seed = 0.0f;
        particles_.push_back(particle);
      }
    }
    ParticleSystemBase::Render(ctx);
  }

  glm::vec4 DashParticleSystem::ParticleColor(const Particle &p, const RenderParams &) const
  {
    float alpha = p.life / std::max(0.001f, p.maxLife);
    return glm::vec4(0.2f, 0.9f, 1.0f, alpha);
  }

  float DashParticleSystem::ParticleRadius(const Particle &p) const
  {
    return (p.life / std::max(0.001f, p.maxLife)) * 0.12f;
  }

} // namespace mecha
