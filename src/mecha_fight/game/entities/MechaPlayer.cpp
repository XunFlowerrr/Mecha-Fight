#include "MechaPlayer.h"

#include "../rendering/RenderConstants.h"
#include "../systems/ProjectileSystem.h"
#include "../systems/MissileSystem.h"
#include "../ui/DeveloperOverlayUI.h"
#include "EnemyDrone.h"
#include "TurretEnemy.h"
#include "GodzillaEnemy.h"
#include "Enemy.h"
#include "../GameplayTypes.h"
#include "../audio/SoundManager.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kWalkingSoundStopDelay = 0.25f; // seconds of inactivity before stopping loop
    constexpr float kPlayerDamageSoundCooldown = 0.25f;
  }

  MechaPlayer::MechaPlayer()
  {
    transform_.position = glm::vec3(0.0f);
    movement_.grounded = true;
    movement_.forwardSpeed = 0.0f;
    movement_.verticalVelocity = 0.0f;
    flight_.currentFuel = kMaxFuel;
    flight_.flying = false;
    combat_.hitPoints = kMaxHP;

    animationController_.RegisterAction(static_cast<int>(ActionState::Idle),
                                        {0, AnimationController::PlaybackMode::StaticPose, false, 0.0f, 1.0f, 0.2f});
    animationController_.RegisterAction(static_cast<int>(ActionState::Walking),
                                        {1, AnimationController::PlaybackMode::LoopingAnimation, false, 0.0f, 1.0f, 0.2f});
    animationController_.RegisterAction(static_cast<int>(ActionState::Flying),
                                        {0, AnimationController::PlaybackMode::LoopingAnimation, true, 0.98f, 1.0f, 0.3f});
    animationController_.RegisterAction(static_cast<int>(ActionState::Attacking),
                                        {0, AnimationController::PlaybackMode::LoopingAnimation, false, 0.0f, 1.0f, 0.1f});
    animationController_.RegisterAction(static_cast<int>(ActionState::Dashing),
                                        {0, AnimationController::PlaybackMode::LoopingAnimation, true, 0.98f, 1.0f, 0.3f});
    // Melee animation - configure clipIndex to match your melee animation clip
    // The animation should play once (non-looping) and have 2 hit frames
    // Using playback window to play animation once from start to end
    animationController_.RegisterAction(static_cast<int>(ActionState::Melee),
                                        {2, AnimationController::PlaybackMode::LoopingAnimation, true, 0.0f, 1.0f, 0.1f});
  }

  MovementState &MechaPlayer::Movement()
  {
    return movement_;
  }

  const MovementState &MechaPlayer::Movement() const
  {
    return movement_;
  }

  FlightState &MechaPlayer::Flight()
  {
    return flight_;
  }

  const FlightState &MechaPlayer::Flight() const
  {
    return flight_;
  }

  BoostState &MechaPlayer::Boost()
  {
    return boost_;
  }

  const BoostState &MechaPlayer::Boost() const
  {
    return boost_;
  }

  CombatState &MechaPlayer::Combat()
  {
    return combat_;
  }

  const CombatState &MechaPlayer::Combat() const
  {
    return combat_;
  }

  WeaponState &MechaPlayer::Weapon()
  {
    return weapon_;
  }

  const WeaponState &MechaPlayer::Weapon() const
  {
    return weapon_;
  }

  MeleeState &MechaPlayer::Melee()
  {
    return melee_;
  }

  const MeleeState &MechaPlayer::Melee() const
  {
    return melee_;
  }

  float &MechaPlayer::ModelScale()
  {
    return modelScale_;
  }

  const float &MechaPlayer::ModelScale() const
  {
    return modelScale_;
  }

  glm::vec3 &MechaPlayer::PivotOffset()
  {
    return pivotOffset_;
  }

  const glm::vec3 &MechaPlayer::PivotOffset() const
  {
    return pivotOffset_;
  }

  void MechaPlayer::SpawnSparkParticles(const glm::vec3 &hitPosition, UpdateParams const *params) const
  {
    if (!params || !params->sparkParticles)
    {
      return;
    }

    constexpr int kSparkCount = 15;
    constexpr float kSparkSpeed = 8.0f;
    constexpr float kSparkLife = 0.5f;

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
      spark.pos = hitPosition + glm::vec3(randSigned() * 0.3f, randFloat() * 0.5f, randSigned() * 0.3f);

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

  void MechaPlayer::SpawnDashParticles(const glm::vec3 &origin, UpdateParams const *params) const
  {
    if (!params || !params->dashParticles)
    {
      return;
    }

    auto &particles = *params->dashParticles;
    for (int i = 0; i < 12; ++i)
    {
      float angle = (static_cast<float>(i) / 12.0f) * 6.28318f;
      glm::vec3 direction(std::cos(angle), 0.5f, std::sin(angle));
      glm::vec3 dashPos = origin + direction * 0.3f;
      glm::vec3 dashVel = glm::normalize(direction) * 15.0f;
      particles.push_back({dashPos, dashVel, 0.4f, 0.4f});
    }
  }

  void MechaPlayer::SpawnDashAfterimage(const glm::vec3 &origin, const glm::vec3 &direction, UpdateParams const *params) const
  {
    if (!params || !params->afterimageParticles)
    {
      return;
    }

    auto &particles = *params->afterimageParticles;
    auto randFloat = []()
    {
      return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };

    // Define spawn points relative to mecha: head, left shoulder, right shoulder, left leg, right leg
    std::array<glm::vec3, 5> relativeOffsets = {
        glm::vec3(0.0f, 1.2f, -0.5f),   // Head
        glm::vec3(-0.8f, 0.8f, -0.5f),  // Left shoulder
        glm::vec3(0.8f, 0.8f, -0.5f),   // Right shoulder
        glm::vec3(-0.8f, -0.5f, -0.5f), // Left leg
        glm::vec3(0.8f, -0.5f, -0.5f)   // Right leg
    };

    for (int i = 0; i < 5; ++i)
    {
      glm::vec3 spawnOffset = origin + relativeOffsets[i] - direction * (0.5f + randFloat() * 0.25f);
      float life = 0.35f + randFloat() * 0.15f;
      float radiusScale = 0.45f + randFloat() * 0.25f;

      AfterimageParticle particle;
      particle.pos = spawnOffset;
      particle.life = life;
      particle.maxLife = life;
      particle.radiusScale = radiusScale;
      particle.intensity = 1.0f;

      particles.push_back(particle);
    }
  }

  void MechaPlayer::SpawnThrusterParticles(const glm::vec3 &mechaBack, UpdateParams const *params, float deltaTime)
  {
    if (!params || !params->thrusterParticles || deltaTime <= 0.0f)
    {
      return;
    }

    auto &particles = *params->thrusterParticles;
    constexpr float kBaseEmissionRate = 10000.0f; // particles per second
    float throttle = glm::clamp((movement_.verticalVelocity + 6.0f) / 12.0f, 0.25f, 1.25f);
    if (boost_.active)
    {
      throttle = glm::min(throttle + 0.5f, 1.75f);
    }

    const float emissionRate = kBaseEmissionRate * throttle;
    thrusterEmissionAccumulator_ += emissionRate * deltaTime;
    int spawnCount = static_cast<int>(thrusterEmissionAccumulator_);
    thrusterEmissionAccumulator_ -= spawnCount;
    spawnCount = std::max(spawnCount, 2);

    const glm::vec3 thrusterOriginCenter = movement_.position + mechaBack * -2.2f + glm::vec3(0.0f, 1.15f, 0.0f);
    glm::vec3 exhaustDir = glm::normalize(mechaBack * 0.6f + glm::vec3(0.0f, -1.2f, 0.0f));
    if (glm::dot(exhaustDir, exhaustDir) < 0.0001f)
    {
      exhaustDir = glm::vec3(0.0f, -1.0f, 0.0f);
    }

    glm::vec3 exhaustRight = glm::cross(exhaustDir, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::dot(exhaustRight, exhaustRight) < 0.0001f)
    {
      exhaustRight = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
      exhaustRight = glm::normalize(exhaustRight);
    }
    const glm::vec3 exhaustUp = glm::normalize(glm::cross(exhaustRight, exhaustDir));

    glm::vec3 thrusterRight = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), mechaBack);
    if (glm::dot(thrusterRight, thrusterRight) < 0.0001f)
    {
      thrusterRight = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    else
    {
      thrusterRight = glm::normalize(thrusterRight);
    }
    constexpr float kNozzleSpacing = 0.35f;
    const std::array<glm::vec3, 2> nozzleOrigins = {
        thrusterOriginCenter + thrusterRight * kNozzleSpacing,
        thrusterOriginCenter - thrusterRight * kNozzleSpacing};

    auto randFloat = []()
    {
      return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };
    auto randSigned = [&]()
    {
      return randFloat() * 2.0f - 1.0f;
    };

    const int perNozzleBase = spawnCount / static_cast<int>(nozzleOrigins.size());
    int remainder = spawnCount % static_cast<int>(nozzleOrigins.size());

    auto spawnFromOrigin = [&](const glm::vec3 &origin, int count)
    {
      for (int i = 0; i < count; ++i)
      {
        float radialOffset = randSigned() * 0.1f;
        float verticalOffset = randSigned() * 0.08f;
        float nozzleDepth = 0.05f + randFloat() * 0.1f;

        ThrusterParticle particle;
        particle.pos = origin + exhaustDir * nozzleDepth + exhaustRight * radialOffset + exhaustUp * verticalOffset;

        float plumeSpeed = 12.0f + randFloat() * 10.0f;
        float swirl = randSigned() * 3.5f;
        float upwardKick = randSigned() * 1.8f;
        particle.vel = exhaustDir * plumeSpeed + exhaustRight * swirl + exhaustUp * upwardKick;

        float life = 0.32f + randFloat() * 0.22f;
        particle.life = life;
        particle.maxLife = life;
        particle.seed = randFloat();
        particle.intensity = 1.15f + randFloat() * 0.8f;
        particle.radiusScale = 0.8f + randFloat() * 0.2f;

        particles.push_back(particle);
      }
    };

    for (size_t nozzleIndex = 0; nozzleIndex < nozzleOrigins.size(); ++nozzleIndex)
    {
      int count = perNozzleBase;
      if (remainder > 0)
      {
        ++count;
        --remainder;
      }
      spawnFromOrigin(nozzleOrigins[nozzleIndex], count);
    }
  }

  void MechaPlayer::Update(const UpdateContext &ctx)
  {
    if (!ctx.window)
    {
      return;
    }

    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    const float deltaTime = ctx.deltaTime;
    damageSoundCooldown_ = std::max(0.0f, damageSoundCooldown_ - deltaTime);
    const bool infiniteFuel = params && params->overlay && params->overlay->infiniteFuel;
    const bool alignToTerrain = (params && params->overlay) ? params->overlay->alignToTerrain : false;
    const bool noclip = (params && params->overlay) ? params->overlay->noclip : false;

    glm::vec3 inputDirection(0.0f);
    if (glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS)
    {
      inputDirection += glm::vec3(1.0f, 0.0f, 0.0f);
    }
    if (glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS)
    {
      inputDirection -= glm::vec3(1.0f, 0.0f, 0.0f);
    }
    if (glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS)
    {
      inputDirection += glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if (glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS)
    {
      inputDirection -= glm::vec3(0.0f, 0.0f, 1.0f);
    }

    if (glm::length(inputDirection) > 0.001f)
    {
      inputDirection = glm::normalize(inputDirection);
    }
    else
    {
      inputDirection = glm::vec3(0.0f);
    }

    // Handle melee input (V key)
    if (glfwGetKey(ctx.window, GLFW_KEY_V) == GLFW_PRESS)
    {
      TryMelee();
    }

    // Update melee state
    UpdateMelee(deltaTime);

    // Update missile cooldown
    if (missile_.cooldown > 0.0f)
    {
      missile_.cooldown -= deltaTime;
      if (missile_.cooldown < 0.0f)
      {
        missile_.cooldown = 0.0f;
      }
    }

    if (!noclip)
    {
      if (boost_.active)
      {
        if (glfwGetKey(ctx.window, GLFW_KEY_LEFT_SHIFT) != GLFW_PRESS)
        {
          boost_.active = false;
          boost_.cooldownLeft = kBoostCooldown;
          boost_.dashPhaseTimeLeft = 0.0f;
          boost_.boostedPhaseTimeLeft = 0.0f;
          boost_.boostTimeLeft = 0.0f;
        }
        else
        {
          boost_.boostTimeLeft -= deltaTime;

          if (boost_.dashPhaseTimeLeft > 0.0f)
          {
            boost_.dashPhaseTimeLeft -= deltaTime;
            if (!infiniteFuel && flight_.currentFuel > 0.0f)
            {
              flight_.currentFuel -= kDashFuelConsumption * deltaTime;
              flight_.currentFuel = std::max(0.0f, flight_.currentFuel);
            }
          }
          else if (boost_.boostedPhaseTimeLeft > 0.0f)
          {
            boost_.boostedPhaseTimeLeft -= deltaTime;
            if (!infiniteFuel && flight_.currentFuel > 0.0f)
            {
              flight_.currentFuel -= kDashFuelConsumption * deltaTime * 0.5f;
              flight_.currentFuel = std::max(0.0f, flight_.currentFuel);
            }
          }

          if (boost_.boostTimeLeft <= 0.0f || flight_.currentFuel <= 0.0f)
          {
            boost_.active = false;
            boost_.cooldownLeft = kBoostCooldown;
          }

          const float radians = glm::radians(movement_.yawDegrees);
          if (glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS)
          {
            boost_.direction = glm::vec3(std::cos(radians), 0.0f, -std::sin(radians));
          }
          else if (glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS)
          {
            boost_.direction = glm::vec3(-std::cos(radians), 0.0f, std::sin(radians));
          }
          else if (glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS)
          {
            boost_.direction = glm::vec3(std::sin(radians), 0.0f, std::cos(radians));
          }
          else if (glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS)
          {
            boost_.direction = glm::vec3(-std::sin(radians), 0.0f, -std::cos(radians));
          }
        }
      }
      else if (boost_.cooldownLeft > 0.0f)
      {
        boost_.cooldownLeft -= deltaTime;
        if (boost_.cooldownLeft < 0.0f)
        {
          boost_.cooldownLeft = 0.0f;
        }
      }

      if (!boost_.active && boost_.cooldownLeft <= 0.0f && flight_.currentFuel > 5.0f &&
          glfwGetKey(ctx.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      {
        boost_.active = true;
        boost_.boostTimeLeft = kDashPhaseDuration + kBoostedSpeedDuration;
        boost_.dashPhaseTimeLeft = kDashPhaseDuration;
        boost_.boostedPhaseTimeLeft = kBoostedSpeedDuration;

        if (glm::length(inputDirection) > 0.001f)
        {
          float radians = glm::radians(movement_.yawDegrees);
          glm::vec3 mechaForward(std::sin(radians), 0.0f, std::cos(radians));
          glm::vec3 mechaRight(std::cos(radians), 0.0f, -std::sin(radians));
          boost_.direction = mechaForward * inputDirection.x - mechaRight * inputDirection.z;
          boost_.direction = glm::normalize(boost_.direction);
        }
        else
        {
          float radians = glm::radians(movement_.yawDegrees);
          boost_.direction = glm::vec3(std::sin(radians), 0.0f, std::cos(radians));
        }

        SpawnDashParticles(movement_.position, params);

        // Play dash sound
        if (params && params->soundManager)
        {
          params->soundManager->PlaySound3D("PLAYER_DASH", movement_.position);
        }
      }
    }
    else
    {
      boost_.active = false;
      boost_.cooldownLeft = 0.0f;
      boost_.dashPhaseTimeLeft = 0.0f;
      boost_.boostedPhaseTimeLeft = 0.0f;
      boost_.boostTimeLeft = 0.0f;
    }

    if (!noclip)
    {
      if (glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_PRESS)
      {
        if (movement_.grounded)
        {
          movement_.verticalVelocity = kJumpForce;
          movement_.grounded = false;
          flight_.flying = true;
        }

        if (!movement_.grounded && flight_.currentFuel > 0.0f)
        {
          flight_.flying = true;
          movement_.verticalVelocity = glm::min(movement_.verticalVelocity + kFlightAccel * deltaTime, 15.0f);
          if (!infiniteFuel)
          {
            flight_.currentFuel -= kFuelConsumption * deltaTime;
            if (flight_.currentFuel < 0.0f)
            {
              flight_.currentFuel = 0.0f;
            }
          }

          float radians = glm::radians(movement_.yawDegrees);
          glm::vec3 mechaBack(-std::sin(radians), 0.0f, -std::cos(radians));
          SpawnThrusterParticles(mechaBack, params, deltaTime);
        }
      }
      else
      {
        if (flight_.flying)
        {
          flight_.flying = false;
          movement_.verticalVelocity = glm::max(movement_.verticalVelocity - kFlightDescent * deltaTime, -15.0f);
        }

        if (!infiniteFuel && !flight_.flying && !boost_.active && flight_.currentFuel < kMaxFuel)
        {
          flight_.currentFuel += kFuelRegenRate * deltaTime;
          if (flight_.currentFuel > kMaxFuel)
          {
            flight_.currentFuel = kMaxFuel;
          }
        }
      }
    }
    else
    {
      flight_.flying = true;
      movement_.grounded = false;
      movement_.verticalVelocity = 0.0f;
      flight_.currentFuel = kMaxFuel;

      // Allow vertical translation while ignoring gravity or collisions.
      float verticalInput = 0.0f;
      if (glfwGetKey(ctx.window, GLFW_KEY_SPACE) == GLFW_PRESS)
      {
        verticalInput += 1.0f;
      }
      if (glfwGetKey(ctx.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
          glfwGetKey(ctx.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
      {
        verticalInput -= 1.0f;
      }

      if (std::abs(verticalInput) > 0.001f)
      {
        movement_.position.y += verticalInput * kNoclipVerticalSpeed * deltaTime;
      }
    }

    if (glfwGetKey(ctx.window, GLFW_KEY_W) == GLFW_PRESS)
    {
      movement_.forwardSpeed += kAcceleration * deltaTime;
      movement_.forwardSpeed = glm::min(movement_.forwardSpeed, kMaxSpeed);
    }
    else if (glfwGetKey(ctx.window, GLFW_KEY_S) == GLFW_PRESS)
    {
      movement_.forwardSpeed -= kAcceleration * deltaTime;
      movement_.forwardSpeed = glm::max(movement_.forwardSpeed, -kMaxSpeed * 0.5f);
    }
    else
    {
      if (movement_.forwardSpeed > 0.0f)
      {
        movement_.forwardSpeed -= kDeceleration * deltaTime;
        movement_.forwardSpeed = std::max(0.0f, movement_.forwardSpeed);
      }
      else if (movement_.forwardSpeed < 0.0f)
      {
        movement_.forwardSpeed += kDeceleration * deltaTime;
        movement_.forwardSpeed = std::min(0.0f, movement_.forwardSpeed);
      }
    }

    float strafeAccel = 0.0f;
    if (glfwGetKey(ctx.window, GLFW_KEY_A) == GLFW_PRESS)
    {
      strafeAccel = kAcceleration * deltaTime;
    }
    if (glfwGetKey(ctx.window, GLFW_KEY_D) == GLFW_PRESS)
    {
      strafeAccel = -kAcceleration * deltaTime;
    }

    if (std::abs(strafeAccel) > 0.001f)
    {
      float radians = glm::radians(movement_.yawDegrees);
      glm::vec3 strafeDir(std::cos(radians), 0.0f, -std::sin(radians));
      float strafeScale = flight_.flying ? 1.5f : 0.8f;
      movement_.position += strafeDir * strafeAccel * strafeScale;
    }

    float radians = glm::radians(movement_.yawDegrees);
    movement_.position.x += std::sin(radians) * movement_.forwardSpeed * deltaTime;
    movement_.position.z += std::cos(radians) * movement_.forwardSpeed * deltaTime;

    if (boost_.active)
    {
      float boostAccel = 0.0f;
      if (boost_.dashPhaseTimeLeft > 0.0f)
      {
        boostAccel = kDashAcceleration;
      }
      else if (boost_.boostedPhaseTimeLeft > 0.0f)
      {
        boostAccel = kBoostSpeedAcceleration;
      }

      if (boostAccel > 0.001f)
      {
        movement_.position += boost_.direction * boostAccel * deltaTime;
      }

      constexpr float kAfterimageInterval = 0.001f;
      afterimageEmissionAccumulator_ += deltaTime;
      while (afterimageEmissionAccumulator_ >= kAfterimageInterval)
      {
        SpawnDashAfterimage(movement_.position, boost_.direction, params);
        afterimageEmissionAccumulator_ -= kAfterimageInterval;
      }
    }
    else
    {
      afterimageEmissionAccumulator_ = 0.0f;
    }

    if (!noclip)
    {
      movement_.verticalVelocity -= kGravity * deltaTime;
      movement_.position.y += movement_.verticalVelocity * deltaTime;

      glm::vec3 mechaForward(std::sin(radians), 0.0f, std::cos(radians));
      glm::vec3 mechaRight(std::cos(radians), 0.0f, -std::sin(radians));

      glm::vec3 wheelFL = movement_.position + mechaForward * kMechaWheelbase + mechaRight * kMechaTrackWidth;
      glm::vec3 wheelFR = movement_.position + mechaForward * kMechaWheelbase - mechaRight * kMechaTrackWidth;
      glm::vec3 wheelRL = movement_.position - mechaForward * kMechaWheelbase + mechaRight * kMechaTrackWidth;
      glm::vec3 wheelRR = movement_.position - mechaForward * kMechaWheelbase - mechaRight * kMechaTrackWidth;

      TerrainHeightSampler sampler = params ? params->terrainSampler : TerrainHeightSampler{};
      float heightFL = sampler(wheelFL.x, wheelFL.z);
      float heightFR = sampler(wheelFR.x, wheelFR.z);
      float heightRL = sampler(wheelRL.x, wheelRL.z);
      float heightRR = sampler(wheelRR.x, wheelRR.z);

      float frontHeight = (heightFL + heightFR) * 0.5f;
      float rearHeight = (heightRL + heightRR) * 0.5f;
      float leftHeight = (heightFL + heightRL) * 0.5f;
      float rightHeight = (heightFR + heightRR) * 0.5f;

      if (alignToTerrain)
      {
        movement_.pitchDegrees = std::atan2(frontHeight - rearHeight, kMechaWheelbase * 2.0f) * 180.0f / 3.14159f;
        movement_.rollDegrees = std::atan2(rightHeight - leftHeight, kMechaTrackWidth * 2.0f) * 180.0f / 3.14159f;
      }
      else
      {
        movement_.pitchDegrees = 0.0f;
        movement_.rollDegrees = 0.0f;
      }

      auto solvePlaneHeight = [&]()
      {
        glm::vec3 pFL(wheelFL.x, heightFL, wheelFL.z);
        glm::vec3 pFR(wheelFR.x, heightFR, wheelFR.z);
        glm::vec3 pRL(wheelRL.x, heightRL, wheelRL.z);
        glm::vec3 v1 = pFR - pFL;
        glm::vec3 v2 = pRL - pFL;
        glm::vec3 normal = glm::cross(v1, v2);
        float normalLenSq = glm::dot(normal, normal);
        if (normalLenSq < 1e-6f)
        {
          return (heightFL + heightFR + heightRL + heightRR) * 0.25f;
        }
        normal /= std::sqrt(normalLenSq);
        if (normal.y < 0.0f)
        {
          normal = -normal;
        }
        if (std::abs(normal.y) < 1e-4f)
        {
          return (heightFL + heightFR + heightRL + heightRR) * 0.25f;
        }
        float d = glm::dot(normal, pFL);
        float y = (d - normal.x * movement_.position.x - normal.z * movement_.position.z) / normal.y;
        if (!std::isfinite(y))
        {
          return (heightFL + heightFR + heightRL + heightRR) * 0.25f;
        }
        return y;
      };

      float surfaceHeight = solvePlaneHeight();
      float targetHeight = surfaceHeight + kHeightOffset;

      if (movement_.position.y <= targetHeight)
      {
        movement_.position.y = targetHeight;
        movement_.verticalVelocity = 0.0f;
        movement_.grounded = true;
        flight_.flying = false;
      }
      else if (movement_.position.y > targetHeight + kGroundThreshold)
      {
        movement_.grounded = false;
      }
    }
    else
    {
      movement_.grounded = false;
      movement_.pitchDegrees = 0.0f;
      movement_.rollDegrees = 0.0f;
    }

    // Trees removed - no tree collision checks performed here

    // Update health regeneration
    UpdateHealthRegen(ctx.deltaTime);

    // Update weapon state
    UpdateWeapon(ctx.deltaTime);

    // Update laser state
    if (params)
    {
      UpdateLaser(ctx.deltaTime, params->enemies);
    }

    hudState_.health = combat_.hitPoints;
    hudState_.maxHealth = kMaxHP;
    hudState_.fuel = flight_.currentFuel;
    hudState_.maxFuel = kMaxFuel;
    hudState_.boostActive = boost_.active;
    hudState_.boostTimeLeft = boost_.boostTimeLeft;
    hudState_.boostDuration = kDashPhaseDuration + kBoostedSpeedDuration;
    hudState_.boostCooldownLeft = boost_.cooldownLeft;
    hudState_.boostCooldown = kBoostCooldown;
    hudState_.flying = flight_.flying;

    const bool hasMovementInput = glm::length(inputDirection) > 0.001f;
    const bool walkingVelocity = std::abs(movement_.forwardSpeed) > 0.1f;
    const bool isFlying = flight_.flying || (!movement_.grounded && std::abs(movement_.verticalVelocity) > 0.2f);
    const bool isUsingThruster = flight_.flying; // Only true when thruster is actively being used
    const bool isDashing = boost_.active;
    const bool isAttacking = weapon_.beamActive || (weapon_.shootCooldown > 0.0f && weapon_.shootCooldown >= kShootCooldown * 0.25f);
    const bool isMelee = melee_.active;
    const bool isWalking = (hasMovementInput || walkingVelocity) && !isFlying && !isDashing && !isMelee;

    // Manage looping flight sound (only when using thruster, not just above ground)
    if (params && params->soundManager)
    {
      if (isUsingThruster && !flightSoundHandle_)
      {
        // Start flight sound
        flightSoundHandle_ = params->soundManager->PlaySound3D("PLAYER_FLIGHT", movement_.position);

        // Stop walking sound when flying starts
        if (walkingSoundHandle_)
        {
          params->soundManager->StopSound(walkingSoundHandle_);
          walkingSoundHandle_ = nullptr;
        }
      }
      else if (!isUsingThruster && flightSoundHandle_)
      {
        // Stop flight sound
        params->soundManager->StopSound(flightSoundHandle_);
        flightSoundHandle_ = nullptr;
      }
      else if (isUsingThruster && flightSoundHandle_)
      {
        // Update flight sound position
        params->soundManager->UpdateSoundPosition(flightSoundHandle_, movement_.position);
      }

      // Manage looping walking sound
      if (isWalking && !isFlying)
      {
        walkingSoundGraceTimer_ = 0.0f;
        if (!walkingSoundHandle_)
        {
          walkingSoundHandle_ = params->soundManager->PlaySound3D("PLAYER_WALKING", movement_.position);
          // Increase playback speed for walking sound (1.5x = 50% faster)
          if (walkingSoundHandle_)
          {
            params->soundManager->SetSoundPitch(walkingSoundHandle_, 1.5f);
          }
        }
        else
        {
          params->soundManager->UpdateSoundPosition(walkingSoundHandle_, movement_.position);
        }
      }
      else
      {
        walkingSoundGraceTimer_ += ctx.deltaTime;
        if (walkingSoundHandle_)
        {
          // Allow a small grace period before stopping to avoid rapid restarts.
          if (walkingSoundGraceTimer_ >= kWalkingSoundStopDelay || isFlying)
          {
            params->soundManager->StopSound(walkingSoundHandle_);
            walkingSoundHandle_ = nullptr;
          }
          else
          {
            // Still update position while in grace period
            params->soundManager->UpdateSoundPosition(walkingSoundHandle_, movement_.position);
          }
        }
      }
    }

    ActionState desiredAction = ActionState::Idle;
    if (isMelee)
    {
      desiredAction = ActionState::Melee;
    }
    else if (isDashing)
    {
      desiredAction = ActionState::Dashing;
    }
    else if (isAttacking)
    {
      desiredAction = ActionState::Attacking;
    }
    else if (isFlying)
    {
      desiredAction = ActionState::Flying;
    }
    else if (isWalking)
    {
      desiredAction = ActionState::Walking;
    }
    if (desiredAction != actionState_)
    {
      actionState_ = desiredAction;
      animationController_.SetAction(static_cast<int>(actionState_));
    }
    animationController_.Update(ctx.deltaTime);
  }

  const MechaPlayer::HudState &MechaPlayer::GetHudState() const
  {
    return hudState_;
  }

  void MechaPlayer::SetTargetLock(bool locked)
  {
    weapon_.targetLocked = locked;
    hudState_.targetLocked = locked;
  }

  void MechaPlayer::SetBeamState(bool active, float cooldown, float cooldownMax)
  {
    hudState_.beamActive = active;
    hudState_.beamCooldown = cooldown;
    hudState_.beamCooldownMax = cooldownMax > 0.0f ? cooldownMax : 1.0f;
  }

  void MechaPlayer::TakeDamage(float damage, bool playDamageSound)
  {
    // God mode: skip damage
    if (godMode_)
    {
      return;
    }

    combat_.hitPoints = std::max(0.0f, combat_.hitPoints - damage);
    combat_.regenTimer = kHPRegenDelay;

    // Spawn spark particles at player position when hit
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->sparkParticles)
    {
      SpawnSparkParticles(movement_.position, params);
    }

    // Play damage sound (throttled)
    if (playDamageSound && params && params->soundManager && damageSoundCooldown_ <= 0.0f)
    {
      params->soundManager->PlaySound3D("PLAYER_DAMAGE", movement_.position);
      damageSoundCooldown_ = kPlayerDamageSoundCooldown;
    }
  }

  void MechaPlayer::ResetHealth()
  {
    combat_.hitPoints = kMaxHP;
    combat_.regenTimer = 0.0f;
  }

  void MechaPlayer::SetGodMode(bool enabled)
  {
    godMode_ = enabled;
  }

  bool MechaPlayer::IsGodMode() const
  {
    return godMode_;
  }

  void MechaPlayer::UpdateWeapon(float deltaTime)
  {
    if (weapon_.shootCooldown > 0.0f)
    {
      weapon_.shootCooldown -= deltaTime;
    }

    if (weapon_.beamActive)
    {
      weapon_.beamTimer -= deltaTime;
      if (weapon_.beamTimer <= 0.0f)
      {
        weapon_.beamActive = false;
      }
    }
  }

  void MechaPlayer::TryShoot(const glm::vec3 &targetPos, const glm::vec3 &targetVel, bool hasTarget,
                             const glm::mat4 &projection, const glm::mat4 &view, ProjectileSystem *projectiles)
  {
    if (weapon_.shootCooldown > 0.0f || !projectiles)
    {
      return;
    }

    glm::vec3 dir;
    glm::vec3 spawn = movement_.position + glm::vec3(0.0f, kSpawnHeightOffset, 0.0f);

    if (weapon_.targetLocked && hasTarget)
    {
      // Auto-aim with leading prediction
      glm::vec3 toTarget = targetPos - spawn;
      float timeToTarget = glm::length(toTarget) / kBulletSpeed;
      glm::vec3 predictedPos = targetPos + targetVel * timeToTarget;
      dir = glm::normalize(predictedPos - spawn);
    }
    else
    {
      // Manual aim from camera center
      glm::vec4 clipCoords(0.0f, 0.0f, -1.0f, 1.0f);
      glm::mat4 invProj = glm::inverse(projection);
      glm::vec4 eyeCoords = invProj * clipCoords;
      eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
      glm::mat4 invView = glm::inverse(view);
      dir = glm::normalize(glm::vec3(invView * eyeCoords));
    }

    dir = glm::normalize(dir + glm::vec3(0.0f, kBulletUpBias, 0.0f));

    projectiles->SpawnPlayerShot(spawn, dir * kBulletSpeed);
    weapon_.shootCooldown = kShootCooldown;
    weapon_.beamActive = true;
    weapon_.beamTimer = kBeamDuration;

    // Play shoot sound
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->soundManager)
    {
      params->soundManager->PlaySound3D("PLAYER_SHOOT", spawn);
    }
  }

  void MechaPlayer::TryLaunchMissiles(const glm::mat4 &projection, const glm::mat4 &view, MissileSystem *missileSystem, const std::vector<Enemy *> &enemies)
  {
    if (!missileSystem || missile_.cooldown > 0.0f)
    {
      return;
    }

    // Find target using wider cone and farther range (similar to auto-aim but with different parameters)
    Enemy *target = nullptr;
    float bestAlignment = -1.0f;

    // Get player's forward direction from camera
    glm::vec4 clipCoords(0.0f, 0.0f, -1.0f, 1.0f);
    glm::mat4 invProj = glm::inverse(projection);
    glm::vec4 eyeCoords = invProj * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 playerForward = glm::normalize(glm::vec3(invView * eyeCoords));

    // Calculate cone threshold (cosine of half the cone angle)
    float coneAngleRad = glm::radians(MissileState::kMissileConeAngleDegrees * 0.5f);
    float coneThreshold = std::cos(coneAngleRad);

    // Check all enemies for target
    for (Enemy *enemy : enemies)
    {
      if (!enemy || !enemy->IsAlive())
      {
        continue;
      }

      glm::vec3 toEnemy = enemy->Position() - movement_.position;
      float dist = glm::length(toEnemy);

      // Check if within missile range
      if (dist >= MissileState::kMissileRange)
      {
        continue;
      }

      // Check if within cone
      glm::vec3 dirToEnemy = glm::normalize(toEnemy);
      float dotProduct = glm::dot(playerForward, dirToEnemy);

      // If within cone and better aligned than current best target
      if (dotProduct >= coneThreshold && dotProduct > bestAlignment)
      {
        bestAlignment = dotProduct;
        target = enemy;
      }
    }

    // Calculate shoulder positions
    float yawRad = glm::radians(movement_.yawDegrees);
    glm::vec3 forwardDir(std::sin(yawRad), 0.0f, std::cos(yawRad));
    glm::vec3 rightDir(std::cos(yawRad), 0.0f, -std::sin(yawRad));
    glm::vec3 upDir(0.0f, 1.0f, 0.0f);

    // Shoulder positions relative to player center
    constexpr float kShoulderHeight = 2.0f;
    constexpr float kShoulderWidth = 0.8f;
    glm::vec3 leftShoulder = movement_.position + rightDir * kShoulderWidth + upDir * kShoulderHeight;
    glm::vec3 rightShoulder = movement_.position - rightDir * kShoulderWidth + upDir * kShoulderHeight;

    // Launch missiles
    missileSystem->LaunchMissiles(leftShoulder, rightShoulder, target);
    missile_.cooldown = MissileState::kMissileCooldown;
  }

  MissileState &MechaPlayer::Missile()
  {
    return missile_;
  }

  const MissileState &MechaPlayer::Missile() const
  {
    return missile_;
  }

  LaserState &MechaPlayer::Laser()
  {
    return laser_;
  }

  const LaserState &MechaPlayer::Laser() const
  {
    return laser_;
  }

  void MechaPlayer::UnlockLaser()
  {
    laser_.unlocked = true;
  }

  void MechaPlayer::UpdateHealthRegen(float deltaTime)
  {
    if (combat_.regenTimer > 0.0f)
    {
      combat_.regenTimer -= deltaTime;
      if (combat_.regenTimer < 0.0f)
      {
        combat_.regenTimer = 0.0f;
      }
    }
    else if (combat_.hitPoints < kMaxHP)
    {
      combat_.hitPoints = glm::min(kMaxHP, combat_.hitPoints + kHPRegenRate * deltaTime);
    }
  }

  void MechaPlayer::Render(const RenderContext &ctx)
  {
    if (!mechaModel_)
    {
      return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, movement_.position);
    model = glm::rotate(model, glm::radians(movement_.yawDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(movement_.pitchDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(movement_.rollDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(modelScale_));
    model = glm::translate(model, -pivotOffset_);

    if (ctx.shadowPass)
    {
      if (!ctx.overrideShader)
      {
        return;
      }

      ctx.overrideShader->setMat4("model", model);
      mechaModel_->Draw(*ctx.overrideShader);
      return;
    }

    if (!mechaShader_)
    {
      return;
    }

    mechaShader_->use();
    mechaShader_->setMat4("projection", ctx.projection);
    mechaShader_->setMat4("view", ctx.view);
    mechaShader_->setMat4("lightSpaceMatrix", ctx.lightSpaceMatrix);
    mechaShader_->setVec3("viewPos", ctx.viewPos);
    mechaShader_->setVec3("lightPos", ctx.lightPos);
    mechaShader_->setVec3("lightIntensity", ctx.lightIntensity);
    mechaShader_->setBool("useBaseColor", false);

    glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
    glBindTexture(GL_TEXTURE_2D, ctx.shadowMapTexture);
    mechaShader_->setInt("shadowMap", kShadowMapTextureUnit);

    bool useSSAO = ctx.ssaoEnabled && ctx.ssaoTexture != 0;
    mechaShader_->setBool("useSSAO", useSSAO);
    mechaShader_->setVec2("screenSize", ctx.screenSize);
    mechaShader_->setFloat("aoStrength", ctx.ssaoStrength);
    if (useSSAO)
    {
      glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
      glBindTexture(GL_TEXTURE_2D, ctx.ssaoTexture);
      mechaShader_->setInt("ssaoMap", kSSAOTexUnit);
    }
    mechaShader_->setMat4("model", model);

    mechaModel_->Draw(*mechaShader_);

    // Render melee hitbox if active
    RenderMeleeHitbox(ctx);

    // Render laser beam if active
    RenderLaserBeam(ctx);
  }

  void MechaPlayer::SetRenderResources(Shader *shader, Model *model)
  {
    mechaShader_ = shader;
    mechaModel_ = model;
    animationController_.BindModel(mechaModel_);
    animationController_.SetAction(static_cast<int>(actionState_));
  }

  void MechaPlayer::SetAnimationControls(bool paused, float speed)
  {
    animationController_.SetControls(paused, speed);
  }

  void MechaPlayer::SetDebugRenderResources(Shader *colorShader, unsigned int sphereVAO, unsigned int sphereIndexCount)
  {
    colorShader_ = colorShader;
    sphereVAO_ = sphereVAO;
    sphereIndexCount_ = sphereIndexCount;

    // Initialize laser beam rendering resources
    if (laserBeamVAO_ == 0)
    {
      glGenVertexArrays(1, &laserBeamVAO_);
      glGenBuffers(1, &laserBeamVBO_);
      glGenBuffers(1, &laserBeamEBO_);

      glBindVertexArray(laserBeamVAO_);
      glBindBuffer(GL_ARRAY_BUFFER, laserBeamVBO_);
      glBufferData(GL_ARRAY_BUFFER, 1024 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
      glEnableVertexAttribArray(0);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, laserBeamEBO_);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, 512 * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

      glBindVertexArray(0);
    }
  }

  void MechaPlayer::RenderMeleeHitbox(const RenderContext &ctx)
  {
    if (ctx.shadowPass || !colorShader_ || sphereVAO_ == 0 || sphereIndexCount_ == 0)
    {
      return;
    }

    // Check if hitbox debug is enabled
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params || !params->overlay || !params->overlay->showMeleeHitbox)
    {
      return;
    }

    // Enable blending for semi-transparent hitbox
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer for transparency

    // Render first hitbox if active
    if (melee_.showHitbox1)
    {
      colorShader_->use();
      colorShader_->setMat4("projection", ctx.projection);
      colorShader_->setMat4("view", ctx.view);

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, melee_.hitbox1Position);
      model = glm::scale(model, glm::vec3(melee_.hitboxRadius));
      colorShader_->setMat4("model", model);
      colorShader_->setVec4("color", glm::vec4(1.0f, 0.0f, 0.0f, 0.6f)); // Red, semi-transparent

      glBindVertexArray(sphereVAO_);
      glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, nullptr);
      glBindVertexArray(0);
    }

    // Render second hitbox if active
    if (melee_.showHitbox2)
    {
      colorShader_->use();
      colorShader_->setMat4("projection", ctx.projection);
      colorShader_->setMat4("view", ctx.view);

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, melee_.hitbox2Position);
      model = glm::scale(model, glm::vec3(melee_.hitboxRadius));
      colorShader_->setMat4("model", model);
      colorShader_->setVec4("color", glm::vec4(1.0f, 0.5f, 0.0f, 0.6f)); // Orange, semi-transparent

      glBindVertexArray(sphereVAO_);
      glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, nullptr);
      glBindVertexArray(0);
    }

    // Restore OpenGL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }

  void MechaPlayer::TryMelee()
  {
    // Can't melee if already meleeing or on cooldown
    if (melee_.active || melee_.cooldown > 0.0f)
    {
      return;
    }

    // Start melee action
    melee_.active = true;
    melee_.timer = 0.0f;
    melee_.hitFrame1Triggered = false;
    melee_.hitFrame2Triggered = false;
    melee_.hitFrame1Damaged = false;
    melee_.hitFrame2Damaged = false;

    // Get actual animation duration
    if (mechaModel_ && mechaModel_->HasAnimations())
    {
      const int meleeClipIndex = 2; // Match the clip index used in RegisterAction
      float clipDuration = mechaModel_->GetAnimationClipDuration(meleeClipIndex);
      if (clipDuration > 0.0f)
      {
        melee_.duration = clipDuration;
      }
    }

    // Play melee sound (initial)
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->soundManager)
    {
      params->soundManager->PlaySound3D("PLAYER_MELEE", movement_.position);
      melee_.meleeSoundHandle_ = params->soundManager->PlaySound3D("PLAYER_MELEE_CONTINUE", movement_.position);
    }

    // Force action state change to ensure animation starts
    // This is needed to break out of any current animation state (like Flying)
    // Temporarily set to a different state first to force the animation controller to reset
    if (actionState_ == ActionState::Melee)
    {
      // If already in melee, temporarily switch to force reset
      actionState_ = ActionState::Idle;
      animationController_.SetAction(static_cast<int>(actionState_));
    }

    // Now set to melee state
    actionState_ = ActionState::Melee;
    animationController_.SetAction(static_cast<int>(actionState_));
  }

  void MechaPlayer::UpdateMelee(float deltaTime)
  {
    // Update cooldown
    if (melee_.cooldown > 0.0f)
    {
      melee_.cooldown -= deltaTime;
      if (melee_.cooldown < 0.0f)
      {
        melee_.cooldown = 0.0f;
      }
    }

    // Update melee timer if active
    if (melee_.active)
    {
      melee_.timer += deltaTime;

      // Check for hit frames
      ProcessMeleeHitFrames();

      // End melee when duration is complete
      if (melee_.timer >= melee_.duration)
      {
        melee_.active = false;
        melee_.timer = 0.0f;
        melee_.cooldown = MeleeState::kMeleeCooldown;
        melee_.hitFrame1Triggered = false;
        melee_.hitFrame2Triggered = false;
        melee_.hitFrame1Damaged = false;
        melee_.hitFrame2Damaged = false;

        // Stop continuing melee sound
        const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
        if (params && params->soundManager && melee_.meleeSoundHandle_)
        {
          params->soundManager->StopSound(melee_.meleeSoundHandle_);
          melee_.meleeSoundHandle_ = nullptr;
        }

        // Action state will be updated in the next Update() call
      }

      // Update continuing melee sound position
      if (melee_.active && melee_.meleeSoundHandle_)
      {
        const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
        if (params && params->soundManager)
        {
          params->soundManager->UpdateSoundPosition(melee_.meleeSoundHandle_, movement_.position);
        }
      }
    }

    // Update hitbox display timers
    if (melee_.hitbox1Timer > 0.0f)
    {
      melee_.hitbox1Timer -= deltaTime;
      if (melee_.hitbox1Timer <= 0.0f)
      {
        melee_.showHitbox1 = false;
        melee_.hitbox1Timer = 0.0f;
      }
    }

    if (melee_.hitbox2Timer > 0.0f)
    {
      melee_.hitbox2Timer -= deltaTime;
      if (melee_.hitbox2Timer <= 0.0f)
      {
        melee_.showHitbox2 = false;
        melee_.hitbox2Timer = 0.0f;
      }
    }
  }

  void MechaPlayer::ProcessMeleeHitFrames()
  {
    if (!melee_.active)
    {
      return;
    }

    // Get enemies from UpdateParams (unified vector)
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }
    const auto &enemies = params->enemies;

    // Calculate normalized progress (0.0 to 1.0)
    float progress = melee_.timer / melee_.duration;
    progress = glm::clamp(progress, 0.0f, 1.0f);

    // Check first hit frame
    if (!melee_.hitFrame1Triggered && progress >= melee_.hitFrame1)
    {
      melee_.hitFrame1Triggered = true;

      // Calculate hitbox position (in front of player)
      float yawRad = glm::radians(movement_.yawDegrees);
      glm::vec3 forwardDir(std::sin(yawRad), 0.0f, std::cos(yawRad));
      melee_.hitbox1Position = movement_.position + forwardDir * 3.0f + glm::vec3(0.0f, 1.5f, 0.0f);
      melee_.showHitbox1 = true;
      melee_.hitbox1Timer = melee_.hitboxDisplayDuration;
      melee_.hitFrame1Damaged = false; // Reset damage flag for this hit frame

      // Check for enemies in hitbox range and apply damage (unified loop)
      for (Enemy *enemy : enemies)
      {
        if (enemy && enemy->IsAlive() && !melee_.hitFrame1Damaged)
        {
          // Special handling for GodzillaEnemy - check guns first
          GodzillaEnemy *godzilla = dynamic_cast<GodzillaEnemy *>(enemy);
          if (godzilla)
          {
            int gunIndex = godzilla->GetGunAtPosition(melee_.hitbox1Position, melee_.hitboxRadius);
            if (gunIndex >= 0)
            {
              constexpr float kMeleeDamage = 25.0f;
              godzilla->ApplyDamageToGun(gunIndex, kMeleeDamage);
              melee_.hitFrame1Damaged = true;
              break;
            }
          }

          const glm::vec3 enemyPos = enemy->Position();
          const float enemyRadius = enemy->Radius();
          const float distance = glm::distance(melee_.hitbox1Position, enemyPos);
          const float hitRange = melee_.hitboxRadius + enemyRadius;

          if (distance <= hitRange)
          {
            constexpr float kMeleeDamage = 25.0f; // Damage per hit frame
            enemy->ApplyDamage(kMeleeDamage);
            melee_.hitFrame1Damaged = true;
            break; // Only damage one enemy per hit frame
          }
        }
      }
    }

    // Check second hit frame
    if (!melee_.hitFrame2Triggered && progress >= melee_.hitFrame2)
    {
      melee_.hitFrame2Triggered = true;

      // Calculate hitbox position (in front of player)
      float yawRad = glm::radians(movement_.yawDegrees);
      glm::vec3 forwardDir(std::sin(yawRad), 0.0f, std::cos(yawRad));
      melee_.hitbox2Position = movement_.position + forwardDir * 3.0f + glm::vec3(0.0f, 1.5f, 0.0f);
      melee_.showHitbox2 = true;
      melee_.hitbox2Timer = melee_.hitboxDisplayDuration;
      melee_.hitFrame2Damaged = false; // Reset damage flag for this hit frame

      // Check for enemies in hitbox range and apply damage (unified loop)
      for (Enemy *enemy : enemies)
      {
        if (enemy && enemy->IsAlive() && !melee_.hitFrame2Damaged)
        {
          // Special handling for GodzillaEnemy - check guns first
          GodzillaEnemy *godzilla = dynamic_cast<GodzillaEnemy *>(enemy);
          if (godzilla)
          {
            int gunIndex = godzilla->GetGunAtPosition(melee_.hitbox2Position, melee_.hitboxRadius);
            if (gunIndex >= 0)
            {
              constexpr float kMeleeDamage = 25.0f;
              godzilla->ApplyDamageToGun(gunIndex, kMeleeDamage);
              melee_.hitFrame2Damaged = true;
              break;
            }
          }

          const glm::vec3 enemyPos = enemy->Position();
          const float enemyRadius = enemy->Radius();
          const float distance = glm::distance(melee_.hitbox2Position, enemyPos);
          const float hitRange = melee_.hitboxRadius + enemyRadius;

          if (distance <= hitRange)
          {
            constexpr float kMeleeDamage = 25.0f; // Damage per hit frame
            enemy->ApplyDamage(kMeleeDamage);
            melee_.hitFrame2Damaged = true;
            break; // Only damage one enemy per hit frame
          }
        }
      }
    }
  }

  void MechaPlayer::TryLaser(const glm::mat4 &projection, const glm::mat4 &view, const std::vector<Enemy *> &enemies)
  {
    if (!laser_.unlocked)
    {
      return; // Laser not unlocked yet
    }

    // Get player's forward direction from camera
    glm::vec4 clipCoords(0.0f, 0.0f, -1.0f, 1.0f);
    glm::mat4 invProj = glm::inverse(projection);
    glm::vec4 eyeCoords = invProj * clipCoords;
    eyeCoords = glm::vec4(eyeCoords.x, eyeCoords.y, -1.0f, 0.0f);
    glm::mat4 invView = glm::inverse(view);
    glm::vec3 playerForward = glm::normalize(glm::vec3(invView * eyeCoords));

    // Calculate cone threshold (cosine of half the cone angle)
    float coneAngleRad = glm::radians(LaserState::kLaserConeAngleDegrees * 0.5f);
    float coneThreshold = std::cos(coneAngleRad);

    // Find best target within cone and range
    Enemy *target = nullptr;
    float bestAlignment = -1.0f;

    for (Enemy *enemy : enemies)
    {
      if (!enemy || !enemy->IsAlive())
      {
        continue;
      }

      glm::vec3 toEnemy = enemy->Position() - movement_.position;
      float dist = glm::length(toEnemy);

      // Check if within laser range
      if (dist >= LaserState::kLaserRange)
      {
        continue;
      }

      // Check if within cone
      glm::vec3 dirToEnemy = glm::normalize(toEnemy);
      float dotProduct = glm::dot(playerForward, dirToEnemy);

      // If within cone and better aligned than current best target
      if (dotProduct >= coneThreshold && dotProduct > bestAlignment)
      {
        bestAlignment = dotProduct;
        target = enemy;
      }
    }

    // Activate laser if we have a target
    if (target)
    {
      laser_.active = true;
      laserTarget_ = target;

      // Start laser sound
      const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
      if (params && params->soundManager && !laserSoundHandle_)
      {
        laserSoundHandle_ = params->soundManager->PlaySound3D("PLAYER_LASER", movement_.position);
      }
    }
    else
    {
      laser_.active = false;
      laserTarget_ = nullptr;

      // Stop laser sound
      const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
      if (params && params->soundManager && laserSoundHandle_)
      {
        params->soundManager->StopSound(laserSoundHandle_);
        laserSoundHandle_ = nullptr;
      }
    }
  }

  void MechaPlayer::UpdateLaser(float deltaTime, const std::vector<Enemy *> &enemies)
  {
    if (!laser_.active || !laser_.unlocked)
    {
      // Stop laser sound if laser is not active
      const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
      if (params && params->soundManager && laserSoundHandle_)
      {
        params->soundManager->StopSound(laserSoundHandle_);
        laserSoundHandle_ = nullptr;
      }
      return;
    }

    // Update laser sound position
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->soundManager && laserSoundHandle_)
    {
      params->soundManager->UpdateSoundPosition(laserSoundHandle_, movement_.position);
    }

    // Update damage timer
    laser_.damageTimer += deltaTime;

    // Apply damage at intervals
    if (laser_.damageTimer >= LaserState::kLaserDamageInterval)
    {
      float damage = LaserState::kLaserDamagePerSecond * laser_.damageTimer;
      laser_.damageTimer = 0.0f;

      // Check if target is still valid
      if (laserTarget_ && laserTarget_->IsAlive())
      {
        // Special handling for GodzillaEnemy - check guns first
        GodzillaEnemy *godzilla = dynamic_cast<GodzillaEnemy *>(laserTarget_);
        if (godzilla)
        {
          // For Godzilla, damage the main body (not guns)
          godzilla->ApplyDamage(damage);
        }
        else
        {
          laserTarget_->ApplyDamage(damage);
        }
      }
      else
      {
        // Target died or became invalid, deactivate laser
        laser_.active = false;
        laserTarget_ = nullptr;
      }
    }

    // Re-check target validity and update if needed
    if (laserTarget_ && (!laserTarget_->IsAlive() ||
                         glm::distance(movement_.position, laserTarget_->Position()) > LaserState::kLaserRange))
    {
      laser_.active = false;
      laserTarget_ = nullptr;
    }
  }

  void MechaPlayer::RenderLaserBeam(const RenderContext &ctx)
  {
    if (ctx.shadowPass || !laser_.active || !laserTarget_ || !colorShader_ || laserBeamVAO_ == 0)
    {
      return;
    }

    // Calculate beam start (player position, slightly above ground)
    glm::vec3 beamStart = movement_.position + glm::vec3(0.0f, kSpawnHeightOffset, 0.0f);
    // Beam end is target position
    glm::vec3 beamEnd = laserTarget_->Position() + glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 direction = beamEnd - beamStart;
    float length = glm::length(direction);
    if (length < 0.01f)
    {
      return; // Too short to render
    }
    direction = direction / length;

    // Create a cylinder mesh for the beam
    constexpr float beamRadius = 0.4f; // Thick beam radius
    constexpr int segments = 8;        // Number of sides for the cylinder

    // Generate cylinder vertices
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve((segments + 1) * 2 * 3); // Two rings (start and end) with 3 floats per vertex
    indices.reserve(segments * 6);            // Two triangles per segment

    // Find perpendicular vectors for cylinder cross-section
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(direction, up));
    if (glm::length(right) < 0.1f)
    {
      // If direction is parallel to up, use a different perpendicular
      right = glm::normalize(glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f)));
    }
    glm::vec3 forward = glm::normalize(glm::cross(right, direction));

    // Generate vertices for start and end rings
    for (int ring = 0; ring < 2; ++ring)
    {
      glm::vec3 center = (ring == 0) ? beamStart : beamEnd;
      for (int i = 0; i <= segments; ++i)
      {
        float angle = 2.0f * 3.14159265359f * static_cast<float>(i) / static_cast<float>(segments);
        glm::vec3 offset = (right * std::cos(angle) + forward * std::sin(angle)) * beamRadius;
        vertices.push_back(center.x + offset.x);
        vertices.push_back(center.y + offset.y);
        vertices.push_back(center.z + offset.z);
      }
    }

    // Generate indices for cylinder sides
    for (int i = 0; i < segments; ++i)
    {
      int startRing = 0;
      int endRing = segments + 1;
      int curr = startRing + i;
      int next = startRing + (i + 1);
      int endCurr = endRing + i;
      int endNext = endRing + (i + 1);

      // First triangle
      indices.push_back(curr);
      indices.push_back(next);
      indices.push_back(endCurr);

      // Second triangle
      indices.push_back(next);
      indices.push_back(endNext);
      indices.push_back(endCurr);
    }

    // Update VBO and EBO
    glBindVertexArray(laserBeamVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, laserBeamVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, laserBeamEBO_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), indices.data());

    // Save OpenGL state
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthMaskEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);

    // Enable blending and disable depth writing for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Render laser beam
    colorShader_->use();
    colorShader_->setMat4("projection", ctx.projection);
    colorShader_->setMat4("view", ctx.view);
    colorShader_->setMat4("model", glm::mat4(1.0f));
    colorShader_->setVec4("color", glm::vec4(0.8f, 0.2f, 1.0f, 0.8f)); // Purple laser beam

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore OpenGL state
    glDepthMask(depthMaskEnabled);
    if (!blendEnabled)
    {
      glDisable(GL_BLEND);
    }
  }

} // namespace mecha
