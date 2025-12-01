#include "TurretEnemy.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/RenderConstants.h"
#include "MechaPlayer.h"
#include "../GameplayTypes.h"
#include "../audio/SoundManager.h"
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kRadius = 1.0f;
    constexpr float kHeightOffset = 2.5f; // Height offset above terrain
    constexpr float kMaxHP = 100.0f;
    constexpr float kRespawnDelay = 3.0f;
    constexpr float kAttackRange = 60.0f; // Range at which turret starts attacking (increased from 30.0f)
    constexpr float kRotationSpeed = 90.0f; // Degrees per second
    constexpr float kDamagePerSecond = 20.0f; // Continuous damage rate
    constexpr float kDamageWindowStart = 0.70f; // 70% of animation
    constexpr float kDamageWindowEnd = 0.80f; // 80% of animation
    constexpr float kIdleWindowStart = 0.0f; // 0% of animation
    constexpr float kIdleWindowEnd = 0.60f; // 60% of animation
    constexpr float kAttackWindowStart = 0.60f; // 60% of animation
    constexpr float kAttackWindowEnd = 1.0f; // 100% of animation
  }

  TurretEnemy::TurretEnemy()
  {
    transform_.position = glm::vec3(0.0f, 0.0f, 20.0f);
    
    // Register idle state: loop 0-60% of animation clip 0
    animationController_.RegisterAction(static_cast<int>(TurretState::Idle),
                                        {0, AnimationController::PlaybackMode::LoopingAnimation, 
                                         true, kIdleWindowStart, kIdleWindowEnd, 0.2f});
    
    // Register attacking state: loop 60-100% of animation clip 0
    animationController_.RegisterAction(static_cast<int>(TurretState::Attacking),
                                        {0, AnimationController::PlaybackMode::LoopingAnimation, 
                                         true, kAttackWindowStart, kAttackWindowEnd, 0.2f});
    
    animationController_.SetControls(false, 1.0f);
    currentState_ = TurretState::Idle;
    animationController_.SetAction(static_cast<int>(currentState_));
  }

  bool TurretEnemy::IsAlive() const
  {
    return alive_;
  }

  float TurretEnemy::Radius() const
  {
    return kRadius;
  }

  const glm::vec3 &TurretEnemy::Position() const
  {
    return transform_.position;
  }

  float TurretEnemy::HitPoints() const
  {
    return hp_;
  }

  void TurretEnemy::ApplyDamage(float amount)
  {
    if (!alive_)
    {
      return;
    }

    hp_ -= amount;
    
    // Spawn spark particles at turret position when hit
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->sparkParticles)
    {
      SpawnSparkParticles(transform_.position, params);
    }
    
    if (hp_ <= 0.0f)
    {
      alive_ = false;
      // Turrets don't respawn - they only spawn once
      currentState_ = TurretState::Idle;
      animationController_.SetAction(static_cast<int>(currentState_));

      // Stop laser sound when turret dies
      if (params && params->soundManager && laserSoundHandle_)
      {
        params->soundManager->StopSound(laserSoundHandle_);
        laserSoundHandle_ = nullptr;
      }

      // Play death sound
      if (params && params->soundManager)
      {
        params->soundManager->PlaySound3D("ENEMY_DEATH", transform_.position);
      }
    }
  }

  void TurretEnemy::SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const
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

  float TurretEnemy::GetAnimationProgress() const
  {
    if (!model_ || !model_->HasAnimations())
    {
      return 0.0f;
    }

    float duration = model_->GetActiveAnimationDuration();
    if (duration <= 0.0f)
    {
      return 0.0f;
    }

    // Get the current animation time within the playback window
    // The model's currentAnimationTime is already within the window range
    // We need to map it back to the full animation normalized time (0-1)
    float windowStart = 0.0f;
    float windowEnd = duration;
    
    if (currentState_ == TurretState::Idle)
    {
      windowStart = duration * kIdleWindowStart;
      windowEnd = duration * kIdleWindowEnd;
    }
    else if (currentState_ == TurretState::Attacking)
    {
      windowStart = duration * kAttackWindowStart;
      windowEnd = duration * kAttackWindowEnd;
    }
    
    // The model's currentAnimationTime should be within [windowStart, windowEnd]
    // We'll approximate by using the window midpoint if we can't access currentAnimationTime directly
    // For now, we'll track progress differently - by using a timer
    // Actually, let's use a simpler approach: track damage window based on state and a timer
    
    // Since we can't easily get the exact animation time, we'll use a different approach:
    // Track when we enter the attack state and use elapsed time to estimate progress
    // For now, return a simple estimate based on state
    if (currentState_ == TurretState::Attacking)
    {
      // Assume we're in the middle of the attack window for damage detection
      // This is a simplification - in a real implementation, we'd track animation time
      return (kDamageWindowStart + kDamageWindowEnd) * 0.5f;
    }
    
    return 0.0f;
  }

  void TurretEnemy::UpdateState(const UpdateParams *params)
  {
    if (!params || !params->player)
    {
      return;
    }

    glm::vec3 toPlayer = params->player->Movement().position - transform_.position;
    float distance = glm::length(glm::vec2(toPlayer.x, toPlayer.z));
    
    TurretState newState = (distance <= kAttackRange) ? TurretState::Attacking : TurretState::Idle;
    
    if (newState != currentState_)
    {
      currentState_ = newState;
      animationController_.SetAction(static_cast<int>(currentState_));
    }
  }

  void TurretEnemy::UpdateRotation(float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player || currentState_ != TurretState::Attacking)
    {
      return;
    }

    glm::vec3 toPlayer = params->player->Movement().position - transform_.position;
    float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z)) + 180.0f; // Add 180 degrees to face correct direction
    
    // Smoothly rotate towards target
    float deltaYaw = targetYaw - yawDegrees_;
    
    // Normalize angle difference to [-180, 180]
    while (deltaYaw > 180.0f)
      deltaYaw -= 360.0f;
    while (deltaYaw < -180.0f)
      deltaYaw += 360.0f;
    
    float maxRotation = kRotationSpeed * deltaTime;
    float rotation = std::clamp(deltaYaw, -maxRotation, maxRotation);
    
    yawDegrees_ += rotation;
    
    // Normalize yaw to [0, 360)
    while (yawDegrees_ >= 360.0f)
      yawDegrees_ -= 360.0f;
    while (yawDegrees_ < 0.0f)
      yawDegrees_ += 360.0f;
  }

  void TurretEnemy::ProcessDamageWindow(float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->player || currentState_ != TurretState::Attacking)
    {
      attackStateTimer_ = 0.0f;
      return;
    }

    if (!model_ || !model_->HasAnimations())
    {
      return;
    }

    float duration = model_->GetActiveAnimationDuration();
    if (duration <= 0.0f)
    {
      return;
    }

    // Calculate the attack window in seconds
    float attackWindowStart = duration * kAttackWindowStart;
    float attackWindowEnd = duration * kAttackWindowEnd;
    float attackWindowDuration = attackWindowEnd - attackWindowStart;
    
    // Calculate damage window within attack window (normalized 0-1 within attack window)
    float damageWindowStartInAttack = (kDamageWindowStart - kAttackWindowStart) / (kAttackWindowEnd - kAttackWindowStart);
    float damageWindowEndInAttack = (kDamageWindowEnd - kAttackWindowStart) / (kAttackWindowEnd - kAttackWindowStart);
    
    // Track time since entering attack state
    if (lastState_ != currentState_)
    {
      attackStateTimer_ = 0.0f;
      lastState_ = currentState_;
    }
    
    if (currentState_ == TurretState::Attacking)
    {
      attackStateTimer_ += deltaTime;
      
      // Calculate progress within attack window (0-1)
      float progressInAttackWindow = std::fmod(attackStateTimer_, attackWindowDuration) / attackWindowDuration;
      
      // Check if we're in the damage window
      inDamageWindow_ = (progressInAttackWindow >= damageWindowStartInAttack && 
                        progressInAttackWindow <= damageWindowEndInAttack);
      
      if (inDamageWindow_)
      {
        // Start looping laser sound if not already playing
        if (params && params->soundManager && !laserSoundHandle_)
        {
          laserSoundHandle_ = params->soundManager->PlaySound3D("ENEMY_TURRET_LASER", transform_.position);
        }
        else if (params && params->soundManager && laserSoundHandle_)
        {
          // Update sound position
          params->soundManager->UpdateSoundPosition(laserSoundHandle_, transform_.position);
        }
        
        // Apply continuous damage
        float damage = kDamagePerSecond * deltaTime;
        const_cast<MechaPlayer*>(params->player)->TakeDamage(damage, false);
      }
      else
      {
        // Stop laser sound when not in damage window
        if (params && params->soundManager && laserSoundHandle_)
        {
          params->soundManager->StopSound(laserSoundHandle_);
          laserSoundHandle_ = nullptr;
        }
      }
    }
    else
    {
      attackStateTimer_ = 0.0f;
      inDamageWindow_ = false;
      
      // Stop laser sound when not attacking
      if (params && params->soundManager && laserSoundHandle_)
      {
        params->soundManager->StopSound(laserSoundHandle_);
        laserSoundHandle_ = nullptr;
      }
    }
  }

  void TurretEnemy::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }

    const float deltaTime = ctx.deltaTime;

    if (alive_)
    {
      // Update terrain height
      transform_.position.y = params->terrainSampler(transform_.position.x, transform_.position.z) + kHeightOffset;
      
      // Update state based on player distance
      UpdateState(params);
      
      // Rotate towards player when attacking
      UpdateRotation(deltaTime, params);
      
      // Process damage window
      ProcessDamageWindow(deltaTime, params);
    }
    // Turrets don't respawn - they only spawn once

    animationController_.Update(deltaTime);
  }

  void TurretEnemy::Render(const RenderContext &ctx)
  {
    if (!alive_)
    {
      return;
    }
    
    if (!model_)
    {
      // Debug: log once if model is missing
      static bool loggedMissingModel = false;
      if (!loggedMissingModel)
      {
        std::cerr << "[TurretEnemy] WARNING: Cannot render - model is null!" << std::endl;
        loggedMissingModel = true;
      }
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
    
    // Render laser beam when attacking and in damage window
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    RenderLaserBeam(ctx, params);
  }
  
  void TurretEnemy::RenderLaserBeam(const RenderContext &ctx, const UpdateParams *params)
  {
    if (ctx.shadowPass || !inDamageWindow_ || !params || !params->player || 
        currentState_ != TurretState::Attacking || !colorShader_ || beamVAO_ == 0)
    {
      return;
    }
    
    // Calculate beam start (turret position, slightly above ground)
    glm::vec3 beamStart = transform_.position + glm::vec3(0.0f, 0.3f, 0.0f);
    // Beam end is player position
    glm::vec3 beamEnd = params->player->Movement().position + glm::vec3(0.0f, 1.0f, 0.0f);
    
    glm::vec3 direction = beamEnd - beamStart;
    float length = glm::length(direction);
    if (length < 0.01f)
    {
      return; // Too short to render
    }
    direction = direction / length;
    
    // Create a cylinder mesh for the beam
    constexpr float beamRadius = 0.3f; // Thick beam radius
    constexpr int segments = 8; // Number of sides for the cylinder
    
    // Generate cylinder vertices
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve((segments + 1) * 2 * 3); // Two rings (start and end) with 3 floats per vertex
    indices.reserve(segments * 6); // Two triangles per segment
    
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
        float angle = (i / static_cast<float>(segments)) * glm::two_pi<float>();
        glm::vec3 offset = (right * std::cos(angle) + forward * std::sin(angle)) * beamRadius;
        vertices.push_back(center.x + offset.x);
        vertices.push_back(center.y + offset.y);
        vertices.push_back(center.z + offset.z);
      }
    }
    
    // Generate indices for cylinder sides
    for (int i = 0; i < segments; ++i)
    {
      int base0 = i;
      int base1 = i + segments + 1;
      int next0 = (i + 1) % (segments + 1);
      int next1 = next0 + segments + 1;
      
      // First triangle
      indices.push_back(base0);
      indices.push_back(base1);
      indices.push_back(next0);
      
      // Second triangle
      indices.push_back(next0);
      indices.push_back(base1);
      indices.push_back(next1);
    }
    
    // Update VBO and EBO
    glBindBuffer(GL_ARRAY_BUFFER, beamVBO_);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, beamEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // Save current OpenGL state
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthMaskEnabled;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskEnabled);
    
    // Enable blending for semi-transparent beam
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer
    glEnable(GL_DEPTH_TEST); // Still test depth to avoid rendering through objects
    
    colorShader_->use();
    colorShader_->setMat4("projection", ctx.projection);
    colorShader_->setMat4("view", ctx.view);
    colorShader_->setMat4("model", glm::mat4(1.0f));
    colorShader_->setVec4("color", glm::vec4(0.0f, 1.0f, 0.3f, 0.7f)); // Bright green laser beam with reduced alpha
    
    glBindVertexArray(beamVAO_);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Restore OpenGL state
    glDepthMask(depthMaskEnabled);
    if (!blendEnabled)
    {
      glDisable(GL_BLEND);
    }
  }

  void TurretEnemy::SetRenderResources(Shader *shader, Model *model, bool useBaseColor, const glm::vec3 &baseColor)
  {
    shader_ = shader;
    model_ = model;
    useBaseColor_ = useBaseColor;
    baseColor_ = baseColor;
    animationController_.BindModel(model);
    
    // Ensure animation is set up after model is bound (same pattern as EnemyDrone)
    if (model && model->HasAnimations())
    {
      int clipCount = model->GetAnimationClipCount();
      std::cout << "[TurretEnemy] Model has " << clipCount << " animation(s)" << std::endl;
      if (clipCount > 0)
      {
        // Explicitly set the action after model is bound to ensure animation starts
        animationController_.SetAction(static_cast<int>(currentState_));
        std::cout << "[TurretEnemy] Set animation action to: " << static_cast<int>(currentState_) 
                  << " (Idle=0, Attacking=1)" << std::endl;
      }
    }
    else
    {
      std::cerr << "[TurretEnemy] WARNING: Model has no animations!" << std::endl;
    }
  }

  void TurretEnemy::SetAnimationControls(bool paused, float speed)
  {
    animationController_.SetControls(paused, speed);
  }

  void TurretEnemy::SetLaserBeamResources(Shader *colorShader)
  {
    colorShader_ = colorShader;
    
    // Create cylinder VAO/VBO/EBO for laser beam
    if (beamVAO_ == 0)
    {
      glGenVertexArrays(1, &beamVAO_);
      glGenBuffers(1, &beamVBO_);
      glGenBuffers(1, &beamEBO_);
      
      glBindVertexArray(beamVAO_);
      glBindBuffer(GL_ARRAY_BUFFER, beamVBO_);
      glBufferData(GL_ARRAY_BUFFER, 1024 * sizeof(float), nullptr, GL_DYNAMIC_DRAW); // Allocate enough space
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
      glEnableVertexAttribArray(0);
      
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, beamEBO_);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, 512 * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);
      
      glBindVertexArray(0);
    }
  }

} // namespace mecha

