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

  class TurretEnemy : public Enemy
  {
  public:
    enum class TurretState : int
    {
      Idle = 0,
      Attacking = 1
    };

    struct UpdateParams
    {
      const MechaPlayer *player{nullptr};
      TerrainHeightSampler terrainSampler{};
      std::vector<SparkParticle> *sparkParticles{nullptr};
      class SoundManager *soundManager{nullptr};
    };

    TurretEnemy();

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, Model *model, bool useBaseColor = false, const glm::vec3 &baseColor = glm::vec3(1.0f));
    void SetAnimationControls(bool paused, float speed);

    bool IsAlive() const override;
    float Radius() const override;
    const glm::vec3 &Position() const override;
    float HitPoints() const override;
    float GetYawDegrees() const { return yawDegrees_; }
    float ModelScale() const { return modelScale_; }
    const glm::vec3 &PivotOffset() const { return pivotOffset_; }

    void ApplyDamage(float amount) override;
    void SetModelScale(float scale) { modelScale_ = scale; }
    void SetPivotOffset(const glm::vec3 &offset) { pivotOffset_ = offset; }
    void SetLaserBeamResources(Shader *colorShader);

  private:
    void SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const;
    void UpdateState(const UpdateParams *params);
    void UpdateRotation(float deltaTime, const UpdateParams *params);
    void ProcessDamageWindow(float deltaTime, const UpdateParams *params);
    float GetAnimationProgress() const;
    void RenderLaserBeam(const RenderContext &ctx, const UpdateParams *params);

    glm::vec3 velocity_{0.0f, 0.0f, 0.0f};
    float hp_{100.0f};
    bool alive_{true};
    float respawnTimer_{0.0f};
    float yawDegrees_{0.0f};
    float modelScale_{1.0f};
    glm::vec3 pivotOffset_{0.0f};
    Shader *shader_{nullptr};
    Model *model_{nullptr};
    bool useBaseColor_{false};
    glm::vec3 baseColor_{1.0f};
    AnimationController animationController_{};
    TurretState currentState_{TurretState::Idle};
    
    // Damage window tracking
    float attackStateTimer_{0.0f};
    TurretState lastState_{TurretState::Idle};
    bool inDamageWindow_{false};
    
    // Laser beam rendering
    Shader *colorShader_{nullptr};
    unsigned int beamVAO_{0};
    unsigned int beamVBO_{0};
    unsigned int beamEBO_{0};
    unsigned int beamIndexCount_{0};
    
    // Sound handle for looping laser sound
    void* laserSoundHandle_{nullptr};
  };

} // namespace mecha

