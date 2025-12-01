#pragma once

#include <glm/glm.hpp>

class Model;
class Shader;

#include "../../core/Entity.h"
#include "Enemy.h"
#include "../GameplayTypes.h"
#include "../animation/AnimationController.h"

namespace mecha
{
  class MechaPlayer;
  class ProjectileSystem;
  class PortalGate;

  class EnemyDrone : public Enemy
  {
  public:
    enum class ActionState : int
    {
      Idle = 0,
      Moving = 1
    };
    struct UpdateParams
    {
      const MechaPlayer *player{nullptr};
      ProjectileSystem *projectiles{nullptr};
      TerrainHeightSampler terrainSampler{};
      std::vector<SparkParticle> *sparkParticles{nullptr};
      class SoundManager *soundManager{nullptr};
    };

    EnemyDrone();

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, Model *model, bool useBaseColor = false, const glm::vec3 &baseColor = glm::vec3(1.0f));
    void SetAnimationControls(bool paused, float speed);

    bool IsAlive() const override;
    float Radius() const override;
    const glm::vec3 &Position() const override;
    const glm::vec3 &Velocity() const;
    float HitPoints() const override;
    float GetYawDegrees() const { return yawDegrees_; }
    float ModelScale() const { return modelScale_; }
    const glm::vec3 &PivotOffset() const { return pivotOffset_; }

    void ApplyDamage(float amount) override;
    void SetModelScale(float scale) { modelScale_ = scale; }
    void SetPivotOffset(const glm::vec3 &offset) { pivotOffset_ = offset; }
    void SetAssociatedGate(PortalGate *gate);
    PortalGate *AssociatedGate() const { return associatedGate_; }

  private:
    void RespawnAwayFromPlayer(const MechaPlayer *player, TerrainHeightSampler sampler);
    void RespawnNearGate(TerrainHeightSampler sampler);
    void SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const;
    PortalGate *associatedGate_{nullptr};
    glm::vec3 homeCenter_{0.0f, 0.0f, 0.0f};

    glm::vec3 velocity_{0.0f, 0.0f, 0.0f};
    float hp_{50.0f};
    bool alive_{true};
    float shootTimer_{0.0f};
    float directionTimer_{0.0f};
    float respawnTimer_{0.0f};
    void* movementSoundHandle_{nullptr}; // Handle for looping movement sound
    float yawDegrees_{0.0f};
    float modelScale_{1.0f};
    glm::vec3 pivotOffset_{0.0f};
    Shader *shader_{nullptr};
    Model *model_{nullptr};
    bool useBaseColor_{false};
    glm::vec3 baseColor_{1.0f};
    AnimationController animationController_{};
    ActionState actionState_{ActionState::Moving};
  };

} // namespace mecha
