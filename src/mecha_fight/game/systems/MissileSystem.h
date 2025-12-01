#pragma once

#include <vector>

#include "../../core/Entity.h"
#include "../GameplayTypes.h"

class Model;

class Shader;

namespace mecha
{
  class MechaPlayer;
  class Enemy;

  class MissileSystem : public Entity
  {
  public:
    struct UpdateParams
    {
      MechaPlayer *player{nullptr};
      std::vector<Enemy *> enemies;
      std::vector<ThrusterParticle> *thrusterParticles{nullptr};
      std::vector<ShockwaveParticle> *shockwaveParticles{nullptr};
      TerrainHeightSampler terrainSampler{};
      class SoundManager *soundManager{nullptr};
    };

    void Update(const UpdateContext &ctx) override;

    void LaunchMissile(const glm::vec3 &position, const glm::vec3 &initialVelocity, Enemy *target, float scale = 1.0f, float damage = 45.0f);
    void LaunchMissiles(const glm::vec3 &leftShoulder, const glm::vec3 &rightShoulder, Enemy *target);

    const std::vector<Missile> &Missiles() const;

    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount);
    void SetMissileRenderResources(Shader *shader, Model *model, float scale, const glm::vec3 &pivot);

    // Upgrade system
    void UpgradeMissiles() { upgraded_ = true; }
    bool IsUpgraded() const { return upgraded_; }

  private:
    void UpdateMissile(Missile &missile, float deltaTime, const UpdateParams *params);
    void SpawnThrusterParticles(Missile &missile, float deltaTime, const UpdateParams *params);
    void SpawnExplosion(const glm::vec3 &position, const UpdateParams *params) const;
    void ExplodeMissile(Missile &missile, const UpdateParams *params);
    void ApplyShockwaveDamage(const UpdateParams *params, float deltaTime);
    void RenderMissileMesh(const RenderContext &ctx, const Missile &missile);
    void RenderMissileMeshShadow(const RenderContext &ctx, const Missile &missile);

    std::vector<Missile> missiles_{};
    Shader *shader_{nullptr};
    unsigned int sphereVAO_{0};
    unsigned int sphereIndexCount_{0};
    Shader *missileShader_{nullptr};
    Model *missileModel_{nullptr};
    float missileScale_{1.0f};
    glm::vec3 missilePivot_{0.0f};
    bool upgraded_{false};  // After second portal destroyed, launches 4 missiles
  };

} // namespace mecha
