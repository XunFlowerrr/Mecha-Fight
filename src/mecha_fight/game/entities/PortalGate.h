#pragma once

#include <glm/glm.hpp>

class Model;
class Shader;

#include "../../core/Entity.h"
#include "Enemy.h"
#include "../GameplayTypes.h"

namespace mecha
{
  class PortalGate : public Enemy
  {
  public:
    struct UpdateParams
    {
      TerrainHeightSampler terrainSampler{};
      std::vector<SparkParticle> *sparkParticles{nullptr};
      class SoundManager *soundManager{nullptr};
    };

    PortalGate();
    ~PortalGate() = default;

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, Model *model, bool useBaseColor = false, const glm::vec3 &baseColor = glm::vec3(1.0f));

    bool IsAlive() const override;
    float Radius() const override;
    const glm::vec3 &Position() const override;
    void ApplyDamage(float amount) override;
    float HitPoints() const override;

    void SetModelScale(float scale) { modelScale_ = scale; }
    void SetPivotOffset(const glm::vec3 &offset) { pivotOffset_ = offset; }
    float ModelScale() const { return modelScale_; }
    const glm::vec3 &PivotOffset() const { return pivotOffset_; }

  private:
    void SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const;

    float hp_{500.0f};
    bool alive_{true};
    float modelScale_{1.0f};
    glm::vec3 pivotOffset_{0.0f};
    Shader *shader_{nullptr};
    Model *model_{nullptr};
    bool useBaseColor_{false};
    glm::vec3 baseColor_{1.0f};
  };

} // namespace mecha

