#pragma once

#include "ParticleSystemBase.h"

namespace mecha
{

  class AfterimageParticleSystem : public ParticleSystemBase
  {
  public:
    void SetParticles(std::vector<AfterimageParticle> *particles);
    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;

  private:
    glm::vec4 ParticleColor(const Particle &p, const RenderParams &params) const override;
    float ParticleRadius(const Particle &p) const override;

    std::vector<AfterimageParticle> *source_{nullptr};
  };

} // namespace mecha
