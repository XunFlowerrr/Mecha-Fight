#pragma once

#include <vector>
#include <glm/glm.hpp>

class Model;
class Shader;

#include "Enemy.h"
#include "../GameplayTypes.h"
#include "../animation/AnimationController.h"

namespace mecha
{

  class MechaPlayer;

  struct BossGun
  {
    glm::vec3 localPosition{0.0f}; // Position relative to boss center
    float yawDegrees_{0.0f};       // Rotation around Y axis
    float hp_{50.0f};              // Gun health
    bool alive_{true};              // Whether gun is functional
    float shootTimer_{0.0f};       // Timer for shooting cooldown
  };

  class GodzillaEnemy : public Enemy
  {
  public:
    static constexpr float kMaxHP = 5000.0f;

    enum class State
    {
      Dormant = 0,
      Spawning,
      Idle,
      Walking,
      Attacking,
      Dying,
      Dead
    };

    struct UpdateParams
    {
      const MechaPlayer *player{nullptr};
      TerrainHeightSampler terrainSampler{};
      std::vector<ShockwaveParticle> *shockwaveParticles{nullptr};
      std::vector<ThrusterParticle> *thrusterParticles{nullptr};
      class ProjectileSystem *projectiles{nullptr};
      class SoundManager *soundManager{nullptr};
    };

    GodzillaEnemy();

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;

    void SetRenderResources(Shader *shader, Model *model);
    void TriggerSpawn(bool forceImmediate = false);
    void SetShockwaveParticles(std::vector<ShockwaveParticle> *particles);
    void SetPivotOffset(const glm::vec3 &offset) { pivotOffset_ = offset; }
    void SetModelScale(float scale) { modelScale_ = scale; }
    void SetSpawnPosition(const glm::vec3 &pos) { spawnPosition_ = pos; }

    // Enemy interface
    bool IsAlive() const override;
    float Radius() const override;
    const glm::vec3 &Position() const override;
    void ApplyDamage(float amount) override;
    float HitPoints() const override;
    float MaxHitPoints() const { return kMaxHP; }

    // Gun targeting - returns gun index if hit, -1 if body hit
    int GetGunAtPosition(const glm::vec3 &worldPos, float maxDistance) const;
    void ApplyDamageToGun(int gunIndex, float amount);
    const std::vector<BossGun> &GetGuns() const { return guns_; }

    State CurrentState() const { return state_; }

  private:
    void UpdateDormant();
    void UpdateSpawning(float deltaTime, const UpdateParams *params);
    void UpdateBehavior(float deltaTime, const UpdateParams *params);
    void UpdateShockwaves(float deltaTime, const UpdateParams *params);
    void UpdateGuns(float deltaTime, const UpdateParams *params);
    void SpawnShockwave();
    void SpawnDeathFireParticles(float deltaTime, const UpdateParams *params);
    float TerrainHeightAt(const UpdateParams *params, const glm::vec3 &worldPos) const;
    void EnterState(State newState);
    void ApplyShockwaveDamage(const ShockwaveParticle &wave, const UpdateParams *params, float deltaTime);
    void InitializeGuns();
    void UpdateGunRotation(BossGun &gun, float deltaTime, const UpdateParams *params);
    void ProcessGunShooting(BossGun &gun, float deltaTime, const UpdateParams *params);
    glm::vec3 GetGunWorldPosition(const BossGun &gun) const;
    void StartMovementSound(const UpdateParams *params);
    void StopMovementSound(const UpdateParams *params);

    AnimationController animationController_{};
    Shader *shader_{nullptr};
    Model *model_{nullptr};
    glm::vec3 pivotOffset_{0.0f};
    float modelScale_{1.0f};

    float hp_{2000.0f};
    bool alive_{false};
    bool active_{false};
    void* movementSoundHandle_{nullptr}; // Handle for looping movement sound
    State state_{State::Dormant};
    float fallVelocity_{0.0f};
    float attackTimer_{0.0f};
    float attackCooldown_{6.0f};
    float damageMultiplier_{1.0f};
    float spawnHeight_{80.0f};
    float landingOffset_{3.0f};
    glm::vec3 spawnPosition_{0.0f};

    std::vector<ShockwaveParticle> *shockwaveParticles_{nullptr};

    // Gun system
    std::vector<BossGun> guns_;

    // Death fire particle accumulator
    float deathFireAccumulator_{0.0f};
  };

} // namespace mecha


