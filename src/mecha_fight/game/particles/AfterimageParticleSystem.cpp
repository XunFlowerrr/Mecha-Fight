#include "AfterimageParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace mecha
{

  void AfterimageParticleSystem::SetParticles(std::vector<AfterimageParticle> *particles)
  {
    source_ = particles;
  }

  void AfterimageParticleSystem::Update(const UpdateContext &ctx)
  {
    if (!source_)
    {
      return;
    }

    const float dt = ctx.deltaTime;
    for (auto &particle : *source_)
    {
      particle.life -= dt;
    }

    source_->erase(std::remove_if(source_->begin(), source_->end(), [](const AfterimageParticle &p)
                                  { return p.life <= 0.0f; }),
                   source_->end());
  }

  void AfterimageParticleSystem::Render(const RenderContext &ctx)
  {
    particles_.clear();
    if (source_)
    {
      for (const auto &src : *source_)
      {
        Particle particle;
        particle.position = src.pos;
        particle.life = src.life;
        particle.maxLife = src.maxLife;
        particle.radiusScale = src.radiusScale;
        particle.intensity = src.intensity;
        particle.seed = 0.0f;
        particles_.push_back(particle);
      }
    }
    ParticleSystemBase::Render(ctx);
  }

  glm::vec4 AfterimageParticleSystem::ParticleColor(const Particle &p, const RenderParams &) const
  {
    float normalizedLife = p.life / std::max(0.001f, p.maxLife);
    float alpha = glm::clamp(static_cast<float>(std::sqrt(glm::clamp(normalizedLife, 0.0f, 1.0f))), 0.0f, 1.0f);
    return glm::vec4(0.65f, 0.55f, 1.0f, alpha * 0.6f);
  }

  float AfterimageParticleSystem::ParticleRadius(const Particle &p) const
  {
    float normalizedLife = p.life / std::max(0.001f, p.maxLife);
    return glm::clamp(normalizedLife, 0.0f, 1.0f) * 0.2f * p.radiusScale;
  }

} // namespace mecha
