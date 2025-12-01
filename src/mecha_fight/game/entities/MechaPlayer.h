#pragma once

#include <vector>

#include <glm/glm.hpp>

class Shader;
class Model;

#include "../../core/Entity.h"
#include "../GameplayTypes.h"
#include "../animation/AnimationController.h"

namespace mecha
{

  struct DeveloperOverlayState;
  class ProjectileSystem;
  class Enemy;

  struct MovementState
  {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    float yawDegrees{0.0f};
    float pitchDegrees{0.0f};
    float rollDegrees{0.0f};
    float forwardSpeed{0.0f};
    float verticalVelocity{0.0f};
    bool grounded{true};
  };

  struct FlightState
  {
    float currentFuel{100.0f};
    bool flying{false};
  };

  struct BoostState
  {
    bool active{false};
    float boostTimeLeft{0.0f};
    float cooldownLeft{0.0f};
    float dashPhaseTimeLeft{0.0f};
    float boostedPhaseTimeLeft{0.0f};
    glm::vec3 direction{0.0f, 0.0f, 0.0f};
  };

  struct CombatState
  {
    float hitPoints{100.0f};
    float regenTimer{0.0f};
  };

  struct WeaponState
  {
    bool beamActive{false};
    float beamTimer{0.0f};
    float shootCooldown{0.0f};
    bool targetLocked{false};
  };

  struct MeleeState
  {
    bool active{false};
    float timer{0.0f};
    float duration{4.0f};   // Total melee animation duration
    float hitFrame1{0.25f}; // First hit frame time (normalized 0-1)
    float hitFrame2{0.8f};  // Second hit frame time (normalized 0-1)
    bool hitFrame1Triggered{false};
    bool hitFrame2Triggered{false};
    float cooldown{0.0f};
    static constexpr float kMeleeCooldown = 0.5f;
    void *meleeSoundHandle_{nullptr}; // Handle for continuing melee sound

    // Hitbox visualization
    bool showHitbox1{false};
    bool showHitbox2{false};
    glm::vec3 hitbox1Position{0.0f};
    glm::vec3 hitbox2Position{0.0f};
    float hitboxRadius{4.0f};          // Hitbox size
    float hitboxDisplayDuration{0.1f}; // How long to show hitbox
    float hitbox1Timer{0.0f};
    float hitbox2Timer{0.0f};

    // Damage tracking to prevent multiple hits per frame
    bool hitFrame1Damaged{false};
    bool hitFrame2Damaged{false};
  };

  struct MissileState
  {
    float cooldown{0.0f};
    static constexpr float kMissileCooldown = 3.0f;
    static constexpr float kMissileRange = 200.0f;            // Farther range than auto-aim
    static constexpr float kMissileConeAngleDegrees = 120.0f; // Wider cone than auto-aim
  };

  struct LaserState
  {
    bool active{false};
    bool unlocked{false};    // Unlocked after destroying 1 portal
    float damageTimer{0.0f}; // Timer for continuous damage application
    static constexpr float kLaserDamagePerSecond = 50.0f;
    static constexpr float kLaserDamageInterval = 0.1f;    // Apply damage every 0.1 seconds
    static constexpr float kLaserRange = 80.0f;            // Laser range
    static constexpr float kLaserConeAngleDegrees = 30.0f; // Narrow cone for laser
  };

  class MechaPlayer : public Entity
  {
  public:
    enum class ActionState : int
    {
      Idle = 0,
      Walking = 1,
      Flying = 2,
      Attacking = 3,
      Dashing = 4,
      Melee = 5
    };

    static constexpr float kMaxFuel = 150.0f;
    static constexpr float kMaxHP = 100.0f;
    static constexpr float kHPRegenDelay = 2.0f; // seconds after hit
    static constexpr float kHPRegenRate = 8.0f;  // hp per second
    static constexpr float kShootCooldown = 0.2f;
    static constexpr float kBeamDuration = 0.06f;
    static constexpr float kBulletUpBias = 0.05f;            // adds upward tilt to shots
    static constexpr float kAutoAimRange = 30.0f;            // world distance to start auto-aim
    static constexpr float kAutoAimConeAngleDegrees = 45.0f; // cone angle in degrees for auto-aim
    static constexpr float kAutoAimDownBias = -1.0f;         // downward bias for auto-aim (aims slightly lower)
    static constexpr float kBulletSpeed = 22.0f;
    static constexpr float kSpawnHeightOffset = 0.5f;
    static constexpr float kMaxSpeed = 10.0f;
    static constexpr float kAcceleration = 5.0f;
    static constexpr float kDeceleration = 8.0f;
    static constexpr float kGravity = 9.8f;
    static constexpr float kJumpForce = 8.0f;
    static constexpr float kFlightAccel = 8.0f;
    static constexpr float kFlightDescent = 5.0f;
    static constexpr float kNoclipVerticalSpeed = 10.0f;
    static constexpr float kFuelConsumption = 30.0f;
    static constexpr float kFuelRegenRate = 20.0f;
    static constexpr float kDashPhaseDuration = 0.5f;
    static constexpr float kBoostedSpeedDuration = 1.5f;
    static constexpr float kDashAcceleration = 20.0f;
    static constexpr float kBoostSpeedAcceleration = 12.0f;
    static constexpr float kDashFuelConsumption = 50.0f;
    static constexpr float kBoostCooldown = 2.0f;
    static constexpr float kMechaWheelbase = 0.3f;
    static constexpr float kMechaTrackWidth = 0.25f;
    static constexpr float kGroundThreshold = 0.1f;
    static constexpr float kHeightOffset = 1.8f;
    static constexpr float kTargetModelHeight = 4.0f;  // desired visual height for the GLTF mecha
    static constexpr float kCameraHeightOffset = 1.5f; // camera height above mecha position

    struct HudState
    {
      float health{0.0f};
      float maxHealth{kMaxHP};
      float fuel{0.0f};
      float maxFuel{kMaxFuel};
      bool boostActive{false};
      float boostTimeLeft{0.0f};
      float boostDuration{kDashPhaseDuration + kBoostedSpeedDuration};
      float boostCooldownLeft{0.0f};
      float boostCooldown{kBoostCooldown};
      bool flying{false};
      bool targetLocked{false};
      bool beamActive{false};
      float beamCooldown{0.0f};
      float beamCooldownMax{1.0f};
    };

    struct UpdateParams
    {
      DeveloperOverlayState *overlay{nullptr};
      TerrainHeightSampler terrainSampler{};
      std::vector<ThrusterParticle> *thrusterParticles{nullptr};
      std::vector<DashParticle> *dashParticles{nullptr};
      std::vector<AfterimageParticle> *afterimageParticles{nullptr};
      std::vector<SparkParticle> *sparkParticles{nullptr};
      std::vector<class Enemy *> enemies; // All enemies for melee hit detection (unified)
      std::vector<ShockwaveParticle> *shockwaveParticles{nullptr};
      class SoundManager *soundManager{nullptr}; // Sound manager for playing sounds
    };

    MechaPlayer();

    MovementState &Movement();
    const MovementState &Movement() const;

    FlightState &Flight();
    const FlightState &Flight() const;

    BoostState &Boost();
    const BoostState &Boost() const;

    CombatState &Combat();
    const CombatState &Combat() const;

    WeaponState &Weapon();
    const WeaponState &Weapon() const;

    MeleeState &Melee();
    const MeleeState &Melee() const;

    MissileState &Missile();
    const MissileState &Missile() const;

    LaserState &Laser();
    const LaserState &Laser() const;
    void UnlockLaser(); // Call when portal is destroyed

    float &ModelScale();
    const float &ModelScale() const;

    glm::vec3 &PivotOffset();
    const glm::vec3 &PivotOffset() const;

    const HudState &GetHudState() const;
    void SetTargetLock(bool locked);
    void SetBeamState(bool active, float cooldown, float cooldownMax);
    void TakeDamage(float damage, bool playDamageSound = true);
    void ResetHealth();            // Restore health to max
    void SetGodMode(bool enabled); // Enable/disable god mode (invincibility)
    bool IsGodMode() const;        // Check if god mode is active
    void UpdateWeapon(float deltaTime);
    void TryShoot(const glm::vec3 &targetPos, const glm::vec3 &targetVel, bool hasTarget, const glm::mat4 &projection, const glm::mat4 &view, ProjectileSystem *projectiles);
    void TryLaunchMissiles(const glm::mat4 &projection, const glm::mat4 &view, class MissileSystem *missileSystem, const std::vector<Enemy *> &enemies);
    void TryLaser(const glm::mat4 &projection, const glm::mat4 &view, const std::vector<Enemy *> &enemies);
    void UpdateLaser(float deltaTime, const std::vector<Enemy *> &enemies);

    void Update(const UpdateContext &ctx) override;
    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, Model *model);
    void SetDebugRenderResources(Shader *colorShader, unsigned int sphereVAO, unsigned int sphereIndexCount);
    void SetAnimationControls(bool paused, float speed);

  private:
    void SpawnDashParticles(const glm::vec3 &origin, UpdateParams const *params) const;
    void SpawnDashAfterimage(const glm::vec3 &origin, const glm::vec3 &direction, UpdateParams const *params) const;
    void SpawnThrusterParticles(const glm::vec3 &mechaBack, UpdateParams const *params, float deltaTime);
    void SpawnSparkParticles(const glm::vec3 &hitPosition, UpdateParams const *params) const;
    void UpdateHealthRegen(float deltaTime);
    void TryMelee();
    void UpdateMelee(float deltaTime);
    void ProcessMeleeHitFrames();
    void RenderMeleeHitbox(const RenderContext &ctx);
    void RenderLaserBeam(const RenderContext &ctx);

    MovementState movement_{};
    FlightState flight_{};
    BoostState boost_{};
    CombatState combat_{};
    WeaponState weapon_{};
    MeleeState melee_{};
    MissileState missile_{};
    LaserState laser_{};
    float modelScale_{1.0f};
    glm::vec3 pivotOffset_{0.0f, 0.0f, 0.0f};
    HudState hudState_{};
    Shader *mechaShader_{nullptr};
    Model *mechaModel_{nullptr};
    AnimationController animationController_{};
    ActionState actionState_{ActionState::Idle};
    float thrusterEmissionAccumulator_{0.0f};
    float afterimageEmissionAccumulator_{0.0f};

    // Debug rendering resources for hitbox
    Shader *colorShader_{nullptr};
    unsigned int sphereVAO_{0};
    unsigned int sphereIndexCount_{0};

    // Laser beam rendering resources
    unsigned int laserBeamVAO_{0};
    unsigned int laserBeamVBO_{0};
    unsigned int laserBeamEBO_{0};
    Enemy *laserTarget_{nullptr}; // Current target being lasered

    // Looping sound handles
    void *flightSoundHandle_{nullptr};  // Handle for looping flight sound
    void *walkingSoundHandle_{nullptr}; // Handle for looping walking sound
    void *laserSoundHandle_{nullptr};   // Handle for looping laser sound
    float walkingSoundGraceTimer_{0.0f};
    float damageSoundCooldown_{0.0f};

    // God mode (invincibility)
    bool godMode_{false};
  };

} // namespace mecha
