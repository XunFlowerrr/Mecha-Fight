#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../../core/Entity.h"
#include "../GameplayTypes.h"

class Shader;

namespace mecha
{

  class ParticleSystemBase : public Entity
  {
  public:
    struct RenderParams
    {
      glm::vec4 baseColor{1.0f};
    };

    ParticleSystemBase() = default;

    void Render(const RenderContext &ctx) override;
    void SetRenderParams(const RenderParams &params);
    void SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount);

  protected:
    struct Particle
    {
      glm::vec3 position{0.0f};
      glm::vec3 velocity{0.0f};
      float life{0.0f};
      float maxLife{0.0f};
      float intensity{1.0f};
      float radiusScale{1.0f};
      float seed{0.0f};
    };

    std::vector<Particle> particles_{};
    virtual glm::vec4 ParticleColor(const Particle &p, const RenderParams &params) const = 0;
    virtual float ParticleRadius(const Particle &p) const = 0;

  private:
    RenderParams renderParams_{};
    Shader *shader_{nullptr};
    unsigned int sphereVAO_{0};
    unsigned int sphereIndexCount_{0};
  };

} // namespace mecha
