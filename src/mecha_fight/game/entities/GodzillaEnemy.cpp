#include "GodzillaEnemy.h"

#include <algorithm>
#include <iostream>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/RenderConstants.h"
#include "../systems/ProjectileSystem.h"
#include "../audio/SoundManager.h"
#include "MechaPlayer.h"
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kRadius = 8.0f;
    constexpr float kGravity = 9.8f;
    constexpr float kHeightOffset = 15.0f; // Height offset above terrain
    constexpr float kWalkSpeed = 2.0f;     // Speed when walking toward player
    constexpr float kStopDistance = 18.0f; // Desired distance before attacking
    constexpr float kShockwaveThickness = 4.5f;
    constexpr float kShockwaveMaxRadius = 90.0f;
    constexpr float kShockwaveSpeed = 25.0f;
    constexpr float kShockwaveDamagePerSecond = 35.0f;
    constexpr float kAttackTriggerDistance = 70.0f; // Increased from 35.0f to increase shockwave attack range

    // Gun system constants
    constexpr float kGunHP = 50.0f;
    constexpr float kGunRadius = 1.5f;         // Collision radius for guns
    constexpr float kGunRotationSpeed = 60.0f; // Degrees per second
    constexpr float kGunShootRange = 90.0f;    // Range at which guns start shooting (wider attack range)
    constexpr float kGunShootInterval = 1.5f;  // Time between shots
    constexpr float kGunBulletSpeed = 15.0f;   // Bullet speed
    constexpr float kGunBulletSize = 0.20f;    // Bigger bullet size for Godzilla guns
  }

  GodzillaEnemy::GodzillaEnemy()
  {
    spawnPosition_ = glm::vec3(0.0f);
    transform_.position = spawnPosition_;

    AnimationController::ActionConfig idleConfig{};
    idleConfig.clipIndex = 0;
    idleConfig.mode = AnimationController::PlaybackMode::LoopingAnimation;

    AnimationController::ActionConfig walkConfig{};
    walkConfig.clipIndex = 0; // Use animation index 0 for walking
    walkConfig.mode = AnimationController::PlaybackMode::LoopingAnimation;

    AnimationController::ActionConfig attackConfig{};
    attackConfig.clipIndex = 0;
    attackConfig.mode = AnimationController::PlaybackMode::LoopingAnimation;

    AnimationController::ActionConfig deathConfig{};
    deathConfig.clipIndex = 0;
    deathConfig.mode = AnimationController::PlaybackMode::StaticPose;

    animationController_.RegisterAction(static_cast<int>(State::Idle), idleConfig);
    animationController_.RegisterAction(static_cast<int>(State::Walking), {0, AnimationController::PlaybackMode::LoopingAnimation, false, 0.0f, 1.0f, 0.3f});
    animationController_.RegisterAction(static_cast<int>(State::Attacking), attackConfig);
    animationController_.RegisterAction(static_cast<int>(State::Dying), deathConfig);
    animationController_.SetControls(false, 1.0f);

    InitializeGuns();
  }

  void GodzillaEnemy::SetShockwaveParticles(std::vector<ShockwaveParticle> *particles)
  {
    shockwaveParticles_ = particles;
  }

  void GodzillaEnemy::SetRenderResources(Shader *shader, Model *model)
  {
    shader_ = shader;
    model_ = model;
    animationController_.BindModel(model_);

    // Debug: Check if model has skins (required for animation)
    if (model_)
    {
      std::cout << "[GodzillaEnemy] Model has skins: " << (model_->HasSkins() ? "YES" : "NO") << std::endl;
      if (!model_->HasSkins())
      {
        std::cout << "[GodzillaEnemy] WARNING: Model has no skins! Animation will not be visible!" << std::endl;
      }
    }

    // Only set action if state has a registered action (not Dormant or Spawning)
    if (state_ == State::Idle || state_ == State::Walking ||
        state_ == State::Attacking || state_ == State::Dying)
    {
      animationController_.SetAction(static_cast<int>(state_));
    }
    else
    {
      // Set to Idle as default if model is bound
      if (model_ && model_->HasAnimations())
      {
        animationController_.SetAction(static_cast<int>(State::Idle));
      }
    }
  }

  void GodzillaEnemy::TriggerSpawn(bool forceImmediate)
  {
    if (active_ && alive_)
    {
      return;
    }

    active_ = true;
    alive_ = true;
    hp_ = kMaxHP;
    fallVelocity_ = 0.0f;
    attackTimer_ = 2.5f;

    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    float groundHeight = spawnPosition_.y;
    if (params)
    {
      groundHeight = params->terrainSampler(spawnPosition_.x, spawnPosition_.z);
    }

    if (forceImmediate)
    {
      EnterState(State::Idle);
      std::cout << "[GodzillaEnemy] Manual spawn triggered (immediate)." << std::endl;
    }
    else
    {
      EnterState(State::Spawning);
      std::cout << "[GodzillaEnemy] Spawn triggered. Model scale: " << modelScale_ << ", landing offset: " << landingOffset_ << std::endl;
    }

    transform_.position = spawnPosition_;
    if (forceImmediate)
    {
      transform_.position.y = groundHeight + landingOffset_;
    }
    else
    {
      transform_.position.y = groundHeight + spawnHeight_;
    }
  }

  bool GodzillaEnemy::IsAlive() const
  {
    return alive_;
  }

  float GodzillaEnemy::Radius() const
  {
    return kRadius;
  }

  const glm::vec3 &GodzillaEnemy::Position() const
  {
    return transform_.position;
  }

  void GodzillaEnemy::ApplyDamage(float amount)
  {
    if (!alive_)
    {
      return;
    }

    hp_ -= amount;
    if (hp_ <= 0.0f)
    {
      hp_ = 0.0f;
      alive_ = false;
      EnterState(State::Dying);
    }
  }

  float GodzillaEnemy::HitPoints() const
  {
    return hp_;
  }

  void GodzillaEnemy::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    const float deltaTime = ctx.deltaTime;

    switch (state_)
    {
    case State::Dormant:
      UpdateDormant();
      break;
    case State::Spawning:
      UpdateSpawning(deltaTime, params);
      break;
    case State::Idle:
    case State::Walking:
    case State::Attacking:
      UpdateBehavior(deltaTime, params);
      break;
    case State::Dying:
    case State::Dead:
      alive_ = false;
      break;
    }

    UpdateShockwaves(deltaTime, params);
    if (active_ && alive_ && state_ != State::Dormant && state_ != State::Spawning)
    {
      UpdateGuns(deltaTime, params);
    }

    // Spawn fire particles when dying/dead
    if (state_ == State::Dying || state_ == State::Dead)
    {
      SpawnDeathFireParticles(deltaTime, params);

      // Play death sound when entering dying state
      if (state_ == State::Dying)
      {
        // Note: We need to add soundManager to UpdateParams
        // For now, we'll handle this in EnterState
      }
    }

    // Update movement sound position
    if (params && params->soundManager && movementSoundHandle_ && state_ == State::Walking)
    {
      params->soundManager->UpdateSoundPosition(movementSoundHandle_, transform_.position);
    }

    animationController_.Update(deltaTime);
  }

  void GodzillaEnemy::UpdateDormant()
  {
    // Remain inactive until triggered
  }

  void GodzillaEnemy::UpdateSpawning(float deltaTime, const UpdateParams *params)
  {
    fallVelocity_ -= kGravity * deltaTime;
    transform_.position.y += fallVelocity_ * deltaTime;

    float ground = TerrainHeightAt(params, transform_.position) + landingOffset_;
    if (transform_.position.y <= ground)
    {
      transform_.position.y = ground;
      fallVelocity_ = 0.0f;
      EnterState(State::Idle);
      std::cout << "[GodzillaEnemy] Landed at Y = " << ground << std::endl;
    }
  }

  void GodzillaEnemy::UpdateBehavior(float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player)
    {
      return;
    }

    glm::vec3 toPlayer = params->player->Movement().position - transform_.position;
    float planarDistance = glm::length(glm::vec2(toPlayer.x, toPlayer.z));

    // Always face the player (add 180 degrees to fix model facing direction)
    if (planarDistance > 0.001f)
    {
      glm::vec2 toPlayer2D(toPlayer.x, toPlayer.z);
      toPlayer2D = glm::normalize(toPlayer2D);
      float targetYaw = glm::degrees(std::atan2(toPlayer2D.x, toPlayer2D.y)) + 65.0f;
      transform_.rotation.y = targetYaw;
    }

    attackTimer_ -= deltaTime;

    bool withinAttackRange = planarDistance <= kAttackTriggerDistance;

    // Check if we should attack
    if (withinAttackRange && attackTimer_ <= 0.0f)
    {
      SpawnShockwave();
      attackTimer_ = attackCooldown_;
      EnterState(State::Attacking);
    }
    else if (state_ == State::Attacking && attackTimer_ > attackCooldown_ - 0.5f)
    {
      // stay in attack state briefly
    }
    else if (planarDistance > kStopDistance)
    {
      // Walk toward player if we're outside the stop distance
      glm::vec2 toPlayer2D(toPlayer.x, toPlayer.z);
      if (glm::length(toPlayer2D) > 0.001f)
      {
        toPlayer2D = glm::normalize(toPlayer2D);
        glm::vec3 moveDirection(toPlayer2D.x, 0.0f, toPlayer2D.y);
        transform_.position += moveDirection * kWalkSpeed * deltaTime;

        EnterState(State::Walking);
      }
      else
      {
        EnterState(State::Idle);
      }
    }
    else
    {
      // Close enough, idle until attack ready
      EnterState(State::Idle);
    }

    // Maintain height offset above terrain
    float terrainHeight = TerrainHeightAt(params, transform_.position);
    transform_.position.y = terrainHeight + kHeightOffset;
  }

  void GodzillaEnemy::UpdateShockwaves(float deltaTime, const UpdateParams *params)
  {
    if (!shockwaveParticles_)
    {
      return;
    }

    for (auto &wave : *shockwaveParticles_)
    {
      if (!wave.active)
      {
        continue;
      }

      wave.radius += wave.expansionSpeed * deltaTime;
      wave.life -= deltaTime;
      ApplyShockwaveDamage(wave, params, deltaTime);

      if (wave.radius >= wave.maxRadius || wave.life <= 0.0f)
      {
        wave.active = false;
      }
    }

    shockwaveParticles_->erase(std::remove_if(shockwaveParticles_->begin(), shockwaveParticles_->end(),
                                              [](const ShockwaveParticle &wave)
                                              { return !wave.active; }),
                               shockwaveParticles_->end());
  }

  void GodzillaEnemy::ApplyShockwaveDamage(const ShockwaveParticle &wave, const UpdateParams *params, float deltaTime)
  {
    if (!params || !params->player || !wave.active)
    {
      return;
    }

    glm::vec3 playerPos = params->player->Movement().position;
    glm::vec2 planar(playerPos.x - wave.center.x, playerPos.z - wave.center.z);
    float distance = glm::length(planar);

    float inner = glm::max(0.0f, wave.radius - wave.thickness * 0.5f);
    float outer = wave.radius + wave.thickness * 0.5f;

    if (distance >= inner && distance <= outer)
    {
      float dmg = wave.damagePerSecond * deltaTime * damageMultiplier_;
      const_cast<MechaPlayer *>(params->player)->TakeDamage(dmg);
    }
  }

  void GodzillaEnemy::StartMovementSound(const UpdateParams *params)
  {
    if (params && params->soundManager && !movementSoundHandle_)
    {
      movementSoundHandle_ = params->soundManager->PlaySound3D("BOSS_MOVEMENT", transform_.position);
    }
  }

  void GodzillaEnemy::StopMovementSound(const UpdateParams *params)
  {
    if (params && params->soundManager && movementSoundHandle_)
    {
      params->soundManager->StopSound(movementSoundHandle_);
      movementSoundHandle_ = nullptr;
    }
  }

  void GodzillaEnemy::SpawnShockwave()
  {
    if (!shockwaveParticles_)
    {
      return;
    }

    // Play shockwave sound
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->soundManager)
    {
      params->soundManager->PlaySound3D("BOSS_SHOCKWAVE", transform_.position);
    }

    ShockwaveParticle wave{};
    wave.center = transform_.position;
    wave.center.y = transform_.position.y;
    wave.radius = 0.0f;
    wave.thickness = kShockwaveThickness;
    wave.expansionSpeed = kShockwaveSpeed;
    wave.maxRadius = kShockwaveMaxRadius;
    wave.maxLife = wave.maxRadius / std::max(1.0f, wave.expansionSpeed);
    wave.life = wave.maxLife;
    wave.damagePerSecond = kShockwaveDamagePerSecond;
    wave.active = true;

    shockwaveParticles_->push_back(wave);
  }

  float GodzillaEnemy::TerrainHeightAt(const UpdateParams *params, const glm::vec3 &worldPos) const
  {
    if (!params)
    {
      return worldPos.y;
    }
    return params->terrainSampler(worldPos.x, worldPos.z);
  }

  void GodzillaEnemy::EnterState(State newState)
  {
    if (state_ == newState)
    {
      return;
    }
    State oldState = state_;
    state_ = newState;
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());

    if (oldState == State::Walking && state_ != State::Walking)
    {
      StopMovementSound(params);
    }
    if (state_ == State::Walking)
    {
      StartMovementSound(params);
    }

    // Reset fire accumulator when entering dying state
    if (state_ == State::Dying)
    {
      deathFireAccumulator_ = 0.0f;

      // Spawn enormous death shockwave with white color
      if (shockwaveParticles_)
      {
        ShockwaveParticle deathWave{};
        deathWave.center = transform_.position;
        deathWave.center.y = transform_.position.y;
        deathWave.radius = 0.0f;
        deathWave.thickness = 10.0f;      // Thick ring
        deathWave.expansionSpeed = 50.0f; // Fast expansion
        deathWave.maxRadius = 200.0f;     // Enormous radius
        deathWave.maxLife = deathWave.maxRadius / std::max(1.0f, deathWave.expansionSpeed);
        deathWave.life = deathWave.maxLife;
        deathWave.damagePerSecond = 0.0f;              // No damage, just visual
        deathWave.color = glm::vec3(1.0f, 1.0f, 1.0f); // White color
        deathWave.active = true;

        shockwaveParticles_->push_back(deathWave);
      }

      // Stop movement sound and play death sound
      StopMovementSound(params);
      if (params && params->soundManager)
      {
        params->soundManager->PlaySound3D("BOSS_DEATH", transform_.position);
      }
    }

    // Only set action if this state has a registered action
    if (state_ == State::Idle || state_ == State::Walking ||
        state_ == State::Attacking || state_ == State::Dying)
    {
      animationController_.SetAction(static_cast<int>(state_));
    }
  }

  void GodzillaEnemy::Render(const RenderContext &ctx)
  {
    if (!model_ || !shader_ || !active_)
    {
      return;
    }

    if (ctx.shadowPass)
    {
      if (!ctx.overrideShader)
      {
        return;
      }

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, transform_.position);
      model = glm::rotate(model, glm::radians(transform_.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
      model = glm::scale(model, glm::vec3(modelScale_));
      model = glm::translate(model, -pivotOffset_);

      ctx.overrideShader->setMat4("model", model);
      model_->Draw(*ctx.overrideShader);
      return;
    }

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);
    shader_->setMat4("lightSpaceMatrix", ctx.lightSpaceMatrix);
    shader_->setVec3("viewPos", ctx.viewPos);
    shader_->setVec3("lightPos", ctx.lightPos);
    shader_->setVec3("lightIntensity", ctx.lightIntensity);

    glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
    glBindTexture(GL_TEXTURE_2D, ctx.shadowMapTexture);
    shader_->setInt("shadowMap", kShadowMapTextureUnit);

    shader_->setBool("useBaseColor", false);
    shader_->setBool("useSSAO", ctx.ssaoEnabled && ctx.ssaoTexture != 0);
    if (ctx.ssaoEnabled && ctx.ssaoTexture != 0)
    {
      glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
      glBindTexture(GL_TEXTURE_2D, ctx.ssaoTexture);
      shader_->setInt("ssaoMap", kSSAOTexUnit);
    }

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, transform_.position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform_.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale_));
    modelMatrix = glm::translate(modelMatrix, -pivotOffset_);

    shader_->setMat4("model", modelMatrix);
    model_->Draw(*shader_);
  }

  void GodzillaEnemy::InitializeGuns()
  {
    // Initialize 50 guns positioned around the boss
    // Positions are relative to boss center, in local space
    guns_.clear();

    // Position guns in multiple circles around the boss at different heights
    constexpr int kNumGuns = 50;
    constexpr float kBaseRadius = 4.0f;
    constexpr float kRadiusVariation = 2.0f;
    constexpr float kBaseHeight = 6.0f;
    constexpr float kHeightVariation = 4.0f;
    constexpr int kHeightLevels = 5;

    for (int i = 0; i < kNumGuns; ++i)
    {
      // Distribute guns in multiple circles
      float angle = (i / static_cast<float>(kNumGuns)) * 2.0f * 3.14159f;

      // Vary radius to create multiple concentric circles
      float radius = kBaseRadius + (i % 3) * kRadiusVariation;

      // Vary height across multiple levels
      float height = kBaseHeight + (i % kHeightLevels) * (kHeightVariation / kHeightLevels);

      BossGun gun;
      gun.localPosition = glm::vec3(
          std::cos(angle) * radius,
          height,
          std::sin(angle) * radius);
      gun.hp_ = kGunHP;
      gun.alive_ = true;
      guns_.push_back(gun);
    }
  }

  void GodzillaEnemy::UpdateGuns(float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player)
    {
      return;
    }

    for (auto &gun : guns_)
    {
      if (!gun.alive_)
      {
        continue;
      }

      UpdateGunRotation(gun, deltaTime, params);
      ProcessGunShooting(gun, deltaTime, params);
    }
  }

  void GodzillaEnemy::UpdateGunRotation(BossGun &gun, float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player)
    {
      return;
    }

    glm::vec3 gunWorldPos = GetGunWorldPosition(gun);
    glm::vec3 toPlayer = params->player->Movement().position - gunWorldPos;
    toPlayer.y = 0.0f; // Only rotate horizontally

    float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));

    // Normalize angle difference to [-180, 180]
    float deltaYaw = targetYaw - gun.yawDegrees_;
    while (deltaYaw > 180.0f)
      deltaYaw -= 360.0f;
    while (deltaYaw < -180.0f)
      deltaYaw += 360.0f;

    float maxRotation = kGunRotationSpeed * deltaTime;
    float rotation = std::clamp(deltaYaw, -maxRotation, maxRotation);

    gun.yawDegrees_ += rotation;

    // Normalize yaw to [0, 360)
    while (gun.yawDegrees_ >= 360.0f)
      gun.yawDegrees_ -= 360.0f;
    while (gun.yawDegrees_ < 0.0f)
      gun.yawDegrees_ += 360.0f;
  }

  void GodzillaEnemy::ProcessGunShooting(BossGun &gun, float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player || !params->projectiles)
    {
      return;
    }

    glm::vec3 gunWorldPos = GetGunWorldPosition(gun);
    glm::vec3 toPlayer = params->player->Movement().position - gunWorldPos;
    float distance = glm::length(toPlayer);

    // Check if player is in range
    if (distance <= kGunShootRange)
    {
      gun.shootTimer_ -= deltaTime;

      // Shoot bullet when timer expires
      if (gun.shootTimer_ <= 0.0f)
      {
        gun.shootTimer_ = kGunShootInterval;

        // Calculate direction to player
        glm::vec3 direction = glm::normalize(toPlayer);

        // Spawn bullet from gun position
        glm::vec3 bulletStart = gunWorldPos + direction * 0.5f; // Slightly offset from gun
        glm::vec3 bulletVelocity = direction * kGunBulletSpeed;

        params->projectiles->SpawnEnemyShot(bulletStart, bulletVelocity, kGunBulletSize);

        if (params->soundManager)
        {
          params->soundManager->PlaySound3D("BOSS_PROJECTILE", gunWorldPos);
        }
      }
    }
    else
    {
      gun.shootTimer_ = 0.0f; // Reset timer when out of range
    }
  }

  glm::vec3 GodzillaEnemy::GetGunWorldPosition(const BossGun &gun) const
  {
    // Transform local position to world space
    glm::mat4 bossTransform = glm::mat4(1.0f);
    bossTransform = glm::translate(bossTransform, transform_.position);
    bossTransform = glm::rotate(bossTransform, glm::radians(transform_.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    bossTransform = glm::scale(bossTransform, glm::vec3(modelScale_));

    glm::vec4 localPos4(gun.localPosition, 1.0f);
    glm::vec4 worldPos4 = bossTransform * localPos4;
    return glm::vec3(worldPos4);
  }

  int GodzillaEnemy::GetGunAtPosition(const glm::vec3 &worldPos, float maxDistance) const
  {
    for (size_t i = 0; i < guns_.size(); ++i)
    {
      if (!guns_[i].alive_)
      {
        continue;
      }

      glm::vec3 gunWorldPos = GetGunWorldPosition(guns_[i]);
      float distance = glm::distance(worldPos, gunWorldPos);
      if (distance <= maxDistance + kGunRadius)
      {
        return static_cast<int>(i);
      }
    }
    return -1; // No gun hit
  }

  void GodzillaEnemy::ApplyDamageToGun(int gunIndex, float amount)
  {
    if (gunIndex < 0 || gunIndex >= static_cast<int>(guns_.size()))
    {
      return;
    }

    BossGun &gun = guns_[gunIndex];
    if (!gun.alive_)
    {
      return;
    }

    gun.hp_ -= amount;
    if (gun.hp_ <= 0.0f)
    {
      gun.hp_ = 0.0f;
      gun.alive_ = false;
    }
  }

  void GodzillaEnemy::SpawnDeathFireParticles(float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->thrusterParticles || deltaTime <= 0.0f)
    {
      return;
    }

    auto &particles = *params->thrusterParticles;
    constexpr float kFireEmissionRate = 5000.0f; // Particles per second (>500 as requested)

    // Accumulate particles to spawn
    deathFireAccumulator_ += kFireEmissionRate * deltaTime;
    int spawnCount = static_cast<int>(deathFireAccumulator_);
    deathFireAccumulator_ -= static_cast<float>(spawnCount);

    if (spawnCount <= 0)
    {
      return;
    }

    auto randFloat = []()
    {
      return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };
    auto randSigned = [&]()
    {
      return randFloat() * 2.0f - 1.0f;
    };

    // Spawn particles from various points around the boss model
    // Use gun positions and random points on the boss body
    constexpr float kBossRadius = kRadius;
    constexpr float kBossHeight = 15.0f; // Approximate boss height

    for (int i = 0; i < spawnCount; ++i)
    {
      ThrusterParticle particle;

      // Choose spawn point: either from a gun position or random point on boss body
      glm::vec3 spawnPos;
      if (!guns_.empty() && randFloat() < 0.3f)
      {
        // 30% chance to spawn from a random gun position
        int gunIndex = std::rand() % static_cast<int>(guns_.size());
        spawnPos = GetGunWorldPosition(guns_[gunIndex]);
      }
      else
      {
        // Spawn from random point on boss body
        float angle = randFloat() * 2.0f * 3.14159f;
        float radius = randFloat() * kBossRadius * 0.8f; // Within 80% of radius
        float height = randFloat() * kBossHeight;

        spawnPos = transform_.position + glm::vec3(
                                             std::cos(angle) * radius,
                                             height,
                                             std::sin(angle) * radius);
      }

      particle.pos = spawnPos;

      // Velocity scatters outward from boss center with upward bias
      glm::vec3 toParticle = spawnPos - transform_.position;
      float dist = glm::length(toParticle);
      glm::vec3 outwardDir = (dist > 0.001f) ? glm::normalize(toParticle) : glm::vec3(randSigned(), 1.0f, randSigned());

      // Add randomness and upward bias
      glm::vec3 velDir = outwardDir + glm::vec3(randSigned() * 0.4f, randFloat() * 0.6f + 0.3f, randSigned() * 0.4f);
      velDir = glm::normalize(velDir);

      constexpr float kFireSpeed = 8.0f;
      particle.vel = velDir * kFireSpeed * (0.7f + randFloat() * 0.8f);

      constexpr float kFireLife = 1.2f;
      particle.life = kFireLife * (0.8f + randFloat() * 0.4f);
      particle.maxLife = particle.life;
      particle.seed = randFloat();
      particle.intensity = 1.2f + randFloat() * 0.6f; // Brighter for fire
      particle.radiusScale = 0.8f + randFloat() * 0.6f;

      particles.push_back(particle);
    }
  }

} // namespace mecha
