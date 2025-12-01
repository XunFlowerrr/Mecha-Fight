#pragma once

#include "ParticleSystemBase.h"

namespace mecha
{

  class ThrusterParticleSystem : public ParticleSystemBase
  {
  public:
    struct UpdateParams
    {
      float gravity{0.0f};
      float drag{2.0f};
      float turbulenceStrength{6.0f};
      float turbulenceFrequency{12.0f};
      float upwardDrift{1.0f};
    };

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;

    void SetParticles(std::vector<ThrusterParticle> *particles);
    void SetUpdateParams(const UpdateParams &params);

  private:
    glm::vec4 ParticleColor(const Particle &p, const RenderParams &params) const override;
    float ParticleRadius(const Particle &p) const override;

    std::vector<ThrusterParticle> *source_{nullptr};
    UpdateParams updateParams_{};
  };

} // namespace mecha
