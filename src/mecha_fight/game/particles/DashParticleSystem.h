#pragma once

#include "ParticleSystemBase.h"

namespace mecha
{

  class DashParticleSystem : public ParticleSystemBase
  {
  public:
    void SetParticles(std::vector<DashParticle> *particles);
    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;

  private:
    glm::vec4 ParticleColor(const Particle &p, const RenderParams &params) const override;
    float ParticleRadius(const Particle &p) const override;

    std::vector<DashParticle> *source_{nullptr};
  };

} // namespace mecha
