#include "EnemyDrone.h"

#include <cstdlib>
#include <cmath>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/RenderConstants.h"
#include "../systems/ProjectileSystem.h"
#include "MechaPlayer.h"
#include "PortalGate.h"
#include "../GameplayTypes.h"
#include "../audio/SoundManager.h"
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kRadius = 0.6f;
    constexpr float kHoverOffset = 2.0f; // keep drone visually above ground
    constexpr float kMaxHP = 50.0f;
    constexpr float kShootInterval = 1.5f;
    constexpr float kRespawnDelay = 2.0f;
    constexpr float kDirectionInterval = 3.0f;
    constexpr float kEnemySpeed = 4.0f;
    constexpr float kArenaRange = 40.0f;
    constexpr float kMinPlayerDistance = 10.0f;
    constexpr float kSpawnExtent = 90.0f;
    constexpr float kEnemyBulletSpeed = 12.0f;
    constexpr int kMaxConcurrentMovementLoops = 3;
    int gActiveMovementLoops = 0;

    void IncrementMovementLoopCount()
    {
      if (gActiveMovementLoops < kMaxConcurrentMovementLoops)
      {
        ++gActiveMovementLoops;
      }
    }

    void DecrementMovementLoopCount()
    {
      if (gActiveMovementLoops > 0)
      {
        --gActiveMovementLoops;
      }
    }

    bool HasMovementSlotAvailable()
    {
      return gActiveMovementLoops < kMaxConcurrentMovementLoops;
    }
  }

  EnemyDrone::EnemyDrone()
  {
    transform_.position = glm::vec3(0.0f, 0.0f, 15.0f);
    homeCenter_ = transform_.position;
    velocity_ = glm::vec3(kEnemySpeed, 0.0f, 0.0f);
    animationController_.RegisterAction(static_cast<int>(ActionState::Idle),
                                        {0, AnimationController::PlaybackMode::StaticPose, false, 0.0f, 1.0f, 0.2f});
    animationController_.RegisterAction(static_cast<int>(ActionState::Moving),
                                        {1, AnimationController::PlaybackMode::LoopingAnimation, false, 0.0f, 1.0f, 0.3f});
    animationController_.SetControls(false, 0.5f); // Reduced from 3.0f to slow down animation
  }

  void EnemyDrone::SetAssociatedGate(PortalGate *gate)
  {
    associatedGate_ = gate;
    if (gate)
    {
      homeCenter_ = gate->Position();
    }
  }

  bool EnemyDrone::IsAlive() const
  {
    return alive_;
  }

  float EnemyDrone::Radius() const
  {
    return kRadius;
  }

  const glm::vec3 &EnemyDrone::Position() const
  {
    return transform_.position;
  }

  const glm::vec3 &EnemyDrone::Velocity() const
  {
    return velocity_;
  }

  float EnemyDrone::HitPoints() const
  {
    return hp_;
  }

  void EnemyDrone::ApplyDamage(float amount)
  {
    if (!alive_)
    {
      return;
    }

    hp_ -= amount;
    
    // Spawn spark particles at enemy position when hit
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->sparkParticles)
    {
      SpawnSparkParticles(transform_.position, params);
    }
    
    if (hp_ <= 0.0f)
    {
      alive_ = false;
      respawnTimer_ = kRespawnDelay;
      velocity_ = glm::vec3(0.0f);
      actionState_ = ActionState::Idle;
      animationController_.SetAction(static_cast<int>(actionState_));

      // Stop movement sound when drone dies
      const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
      if (params && params->soundManager)
      {
        if (movementSoundHandle_)
        {
          params->soundManager->StopSound(movementSoundHandle_);
          movementSoundHandle_ = nullptr;
          DecrementMovementLoopCount();
        }
        params->soundManager->PlaySound3D("ENEMY_DEATH", transform_.position);
      }
    }
  }

  void EnemyDrone::RespawnAwayFromPlayer(const MechaPlayer *player, TerrainHeightSampler sampler)
  {
    glm::vec3 candidate = transform_.position;
    for (int i = 0; i < 50; ++i)
    {
      float rx = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * (kSpawnExtent * 2.0f);
      float rz = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * (kSpawnExtent * 2.0f);
      if (player)
      {
        glm::vec2 delta(rx - player->Movement().position.x, rz - player->Movement().position.z);
        if (glm::length(delta) < kMinPlayerDistance)
        {
          continue;
        }
      }

      candidate = glm::vec3(rx, sampler(rx, rz) + kRadius + kHoverOffset, rz);
      break;
    }

    transform_.position = candidate;
    hp_ = kMaxHP;
    alive_ = true;
    respawnTimer_ = 0.0f;
    shootTimer_ = 0.0f;
    directionTimer_ = 0.0f;
    float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * glm::two_pi<float>();
    velocity_ = glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * kEnemySpeed;
    actionState_ = ActionState::Moving;
    animationController_.SetAction(static_cast<int>(actionState_));
  }

void EnemyDrone::RespawnNearGate(TerrainHeightSampler sampler)
{
  if (!associatedGate_ || !associatedGate_->IsAlive())
  {
    return;
  }

  constexpr float kSpawnRadius = 15.0f;
  glm::vec3 gatePos = associatedGate_->Position();
  homeCenter_ = gatePos;
  glm::vec3 candidate = transform_.position;

  for (int i = 0; i < 50; ++i)
  {
    float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * glm::two_pi<float>();
    float radius = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * kSpawnRadius;
    float rx = gatePos.x + std::cos(angle) * radius;
    float rz = gatePos.z + std::sin(angle) * radius;

      candidate = glm::vec3(rx, sampler(rx, rz) + kRadius + kHoverOffset, rz);
      break;
    }

    transform_.position = candidate;
    hp_ = kMaxHP;
    alive_ = true;
    respawnTimer_ = 0.0f;
    shootTimer_ = 0.0f;
    directionTimer_ = 0.0f;
    float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * glm::two_pi<float>();
    velocity_ = glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * kEnemySpeed;
    actionState_ = ActionState::Moving;
    animationController_.SetAction(static_cast<int>(actionState_));
  }

  void EnemyDrone::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }

    const float deltaTime = ctx.deltaTime;

    if (alive_)
    {
      // If associated gate is destroyed, kill this drone
      if (associatedGate_ && !associatedGate_->IsAlive())
      {
        alive_ = false;
        hp_ = 0.0f;
        
        // Stop movement sound when gate is destroyed
        if (params && params->soundManager && movementSoundHandle_)
        {
          params->soundManager->StopSound(movementSoundHandle_);
          movementSoundHandle_ = nullptr;
          DecrementMovementLoopCount();
        }
        return;
      }
      
      transform_.position += velocity_ * deltaTime;

      // Update yaw to face movement direction
      if (glm::length(velocity_) > 0.01f)
      {
        yawDegrees_ = glm::degrees(std::atan2(velocity_.x, velocity_.z));
      }

      directionTimer_ += deltaTime;
      if (directionTimer_ >= kDirectionInterval)
      {
        directionTimer_ = 0.0f;
        float angle = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * glm::two_pi<float>();
        velocity_ = glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * kEnemySpeed;
      }

      glm::vec2 relativeToHome(transform_.position.x - homeCenter_.x,
                               transform_.position.z - homeCenter_.z);
      float planarLen = glm::length(relativeToHome);
      if (planarLen > kArenaRange)
      {
        float angle = std::atan2(relativeToHome.y, relativeToHome.x);
        transform_.position.x = homeCenter_.x + std::cos(angle) * kArenaRange * 0.8f;
        transform_.position.z = homeCenter_.z + std::sin(angle) * kArenaRange * 0.8f;
        velocity_ = -velocity_;
      }

      transform_.position.y = params->terrainSampler(transform_.position.x, transform_.position.z) + kRadius + kHoverOffset;

      shootTimer_ += deltaTime;
      if (shootTimer_ >= kShootInterval && params->projectiles && params->player)
      {
        shootTimer_ = 0.0f;
        glm::vec3 dir = glm::normalize(params->player->Movement().position - transform_.position);
        params->projectiles->SpawnEnemyShot(transform_.position + dir * (kRadius + 0.05f), dir * kEnemyBulletSpeed);

        // Play shoot sound
        if (params->soundManager)
        {
          params->soundManager->PlaySound3D("ENEMY_SHOOT", transform_.position);
        }
      }
    }
    else
    {
      if (!associatedGate_ || !associatedGate_->IsAlive())
      {
        return;
      }

      if (respawnTimer_ > 0.0f)
      {
        respawnTimer_ -= deltaTime;
      }
      if (respawnTimer_ <= 0.0f)
      {
        RespawnNearGate(params->terrainSampler);
      }
    }

    const bool hasVelocity = glm::length(velocity_) > 0.1f;
    ActionState desiredAction = (alive_ && hasVelocity) ? ActionState::Moving : ActionState::Idle;
    if (desiredAction != actionState_)
    {
      actionState_ = desiredAction;
      animationController_.SetAction(static_cast<int>(actionState_));
    }
    
    // Manage looping movement sound (limit concurrent loops to avoid stacking)
    if (params && params->soundManager)
    {
      if (alive_ && hasVelocity && !movementSoundHandle_ && HasMovementSlotAvailable())
      {
        movementSoundHandle_ = params->soundManager->PlaySound3D("ENEMY_DRONE_MOVEMENT", transform_.position);
        if (movementSoundHandle_)
        {
          IncrementMovementLoopCount();
        }
      }
      else if ((!alive_ || !hasVelocity) && movementSoundHandle_)
      {
        params->soundManager->StopSound(movementSoundHandle_);
        movementSoundHandle_ = nullptr;
        DecrementMovementLoopCount();
      }
      else if (alive_ && hasVelocity && movementSoundHandle_)
      {
        params->soundManager->UpdateSoundPosition(movementSoundHandle_, transform_.position);
      }
    }
    
    animationController_.Update(deltaTime);
  }

  void EnemyDrone::Render(const RenderContext &ctx)
  {
    if (!alive_ || !model_)
    {
      return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform_.position);
    model = glm::rotate(model, glm::radians(yawDegrees_), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(modelScale_));
    model = glm::translate(model, -pivotOffset_);

    if (ctx.shadowPass)
    {
      if (!ctx.overrideShader)
      {
        return;
      }

      ctx.overrideShader->setMat4("model", model);
      model_->Draw(*ctx.overrideShader);
      return;
    }

    if (!shader_)
    {
      return;
    }

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);
    shader_->setMat4("lightSpaceMatrix", ctx.lightSpaceMatrix);
    shader_->setVec3("viewPos", ctx.viewPos);
    shader_->setVec3("lightPos", ctx.lightPos);
    shader_->setVec3("lightIntensity", ctx.lightIntensity);
    shader_->setBool("useBaseColor", useBaseColor_);
    if (useBaseColor_)
    {
      shader_->setVec3("baseColor", baseColor_);
    }

    glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
    glBindTexture(GL_TEXTURE_2D, ctx.shadowMapTexture);
    shader_->setInt("shadowMap", kShadowMapTextureUnit);

    bool useSSAO = ctx.ssaoEnabled && ctx.ssaoTexture != 0;
    shader_->setBool("useSSAO", useSSAO);
    shader_->setVec2("screenSize", ctx.screenSize);
    shader_->setFloat("aoStrength", ctx.ssaoStrength);
    if (useSSAO)
    {
      glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
      glBindTexture(GL_TEXTURE_2D, ctx.ssaoTexture);
      shader_->setInt("ssaoMap", kSSAOTexUnit);
    }
    shader_->setMat4("model", model);

    model_->Draw(*shader_);
  }

  void EnemyDrone::SetRenderResources(Shader *shader, Model *model, bool useBaseColor, const glm::vec3 &baseColor)
  {
    shader_ = shader;
    model_ = model;
    useBaseColor_ = useBaseColor;
    baseColor_ = baseColor;
    animationController_.BindModel(model_);
    animationController_.SetAction(static_cast<int>(actionState_));

    if (model_ && model_->HasAnimations())
    {
      static bool loggedHexapodInfo = false;
      if (!loggedHexapodInfo)
      {
        int clipCount = model_->GetAnimationClipCount();
        bool hasSkins = model_->HasSkins();
        std::cout << "[EnemyDrone] Model has " << clipCount << " animation(s), HasSkins: " << (hasSkins ? "YES" : "NO") << std::endl;
        if (clipCount > 1)
        {
          std::cout << "[EnemyDrone] Moving action mapped to animation index 1" << std::endl;
        }
        loggedHexapodInfo = true;
      }
    }
  }

  void EnemyDrone::SetAnimationControls(bool paused, float speed)
  {
    animationController_.SetControls(paused, speed);
  }

  void EnemyDrone::SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const
  {
    if (!params || !params->sparkParticles)
    {
      return;
    }

    constexpr int kSparkCount = 12;
    constexpr float kSparkSpeed = 6.0f;
    constexpr float kSparkLife = 0.4f;

    auto randFloat = []()
    {
      return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };
    auto randSigned = [&]()
    {
      return randFloat() * 2.0f - 1.0f;
    };

    auto &particles = *params->sparkParticles;
    for (int i = 0; i < kSparkCount; ++i)
    {
      SparkParticle spark;
      spark.pos = hitPosition + glm::vec3(randSigned() * 0.25f, randFloat() * 0.4f, randSigned() * 0.25f);
      
      // Random velocity in all directions
      glm::vec3 direction(randSigned(), randFloat() * 0.5f + 0.5f, randSigned());
      direction = glm::normalize(direction);
      spark.vel = direction * (kSparkSpeed * (0.7f + randFloat() * 0.6f));
      
      spark.life = kSparkLife * (0.8f + randFloat() * 0.4f);
      spark.maxLife = spark.life;
      spark.seed = randFloat();
      
      particles.push_back(spark);
    }
  }

} // namespace mecha
