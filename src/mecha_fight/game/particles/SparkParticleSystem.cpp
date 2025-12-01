#include "SparkParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace mecha
{

  void SparkParticleSystem::SetParticles(std::vector<SparkParticle> *particles)
  {
    source_ = particles;
  }

  void SparkParticleSystem::Update(const UpdateContext &ctx)
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
      // Apply gravity and drag
      p.vel.y -= 9.8f * dt; // Gravity
      p.vel *= 0.92f; // Drag
    }

    source_->erase(std::remove_if(source_->begin(), source_->end(), [](const SparkParticle &p)
                                  { return p.life <= 0.0f; }),
                   source_->end());
  }

  void SparkParticleSystem::Render(const RenderContext &ctx)
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
        particle.seed = src.seed;
        particles_.push_back(particle);
      }
    }
    ParticleSystemBase::Render(ctx);
  }

  glm::vec4 SparkParticleSystem::ParticleColor(const Particle &p, const RenderParams &) const
  {
    float alpha = p.life / std::max(0.001f, p.maxLife);
    // Orange/yellow spark color that fades
    float t = 1.0f - alpha;
    glm::vec3 color = glm::mix(glm::vec3(1.0f, 0.8f, 0.2f), glm::vec3(1.0f, 0.4f, 0.1f), t);
    return glm::vec4(color, alpha * 0.9f);
  }

  float SparkParticleSystem::ParticleRadius(const Particle &p) const
  {
    float lifeRatio = p.life / std::max(0.001f, p.maxLife);
    return lifeRatio * 0.15f;
  }

} // namespace mecha

