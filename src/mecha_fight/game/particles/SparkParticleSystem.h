#pragma once

#include "ParticleSystemBase.h"

namespace mecha
{

  class SparkParticleSystem : public ParticleSystemBase
  {
  public:
    void SetParticles(std::vector<SparkParticle> *particles);
    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;

  private:
    glm::vec4 ParticleColor(const Particle &p, const RenderParams &params) const override;
    float ParticleRadius(const Particle &p) const override;

    std::vector<SparkParticle> *source_{nullptr};
  };

} // namespace mecha

