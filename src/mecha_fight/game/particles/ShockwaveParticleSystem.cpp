#include "ShockwaveParticleSystem.h"

namespace mecha
{

  void ShockwaveParticleSystem::SetParticles(std::vector<ShockwaveParticle> *particles)
  {
    source_ = particles;
  }

  void ShockwaveParticleSystem::Update(const UpdateContext &)
  {
    // Shockwave progression is handled by the spawning entity (e.g., GodzillaEnemy)
    // This system only renders whatever is in the shared vector.
  }

  void ShockwaveParticleSystem::Render(const RenderContext &ctx)
  {
    particles_.clear();
    if (source_)
    {
      for (const auto &wave : *source_)
      {
        if (!wave.active)
        {
          continue;
        }

        Particle particle;
        particle.position = wave.center;
        particle.life = wave.life;
        particle.maxLife = wave.maxLife;
        particle.radiusScale = wave.radius;
        // Store color in seed field (encode RGB as floats, we'll decode in ParticleColor)
        particle.seed = wave.color.r * 1000.0f + wave.color.g * 10.0f + wave.color.b; // Simple encoding
        particles_.push_back(particle);
      }
    }

    ParticleSystemBase::Render(ctx);
  }

  glm::vec4 ShockwaveParticleSystem::ParticleColor(const Particle &p, const RenderParams &) const
  {
    // Try to find matching wave by position to get custom color
    if (source_)
    {
      for (const auto &wave : *source_)
      {
        if (!wave.active)
        {
          continue;
        }
        // Match by position (with small tolerance)
        if (glm::distance(p.position, wave.center) < 0.1f)
        {
          // High intensity for white shockwaves (detected by checking if color is close to white)
          float alpha = 0.35f;
          if (wave.color.r > 0.9f && wave.color.g > 0.9f && wave.color.b > 0.9f)
          {
            alpha = 0.85f; // High intensity white shockwave
          }
          return glm::vec4(wave.color, alpha);
        }
      }
    }
    // Default bright cyan ring color with subtle alpha falloff
    return glm::vec4(0.3f, 0.9f, 0.9f, 0.35f);
  }

  float ShockwaveParticleSystem::ParticleRadius(const Particle &p) const
  {
    return glm::max(0.1f, p.radiusScale);
  }

} // namespace mecha


