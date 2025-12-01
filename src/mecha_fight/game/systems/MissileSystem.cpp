#include "MissileSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../entities/MechaPlayer.h"
#include "../entities/Enemy.h"
#include "../rendering/RenderConstants.h"
#include "../audio/SoundManager.h"
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kMissileSpeed = 25.0f;
    constexpr float kMissileHomingStrength = 15.0f; // How strongly missile turns toward target (increased from 8)
    constexpr float kMissileMaxTurnRate = 280.0f;  // Degrees per second max turn rate (increased from 180)
    constexpr float kMissileExplosionRadius = 8.0f;
    constexpr float kMissileExplosionDamage = 80.0f;
    constexpr float kMissileDirectDamage = 45.0f;
    constexpr float kMiniMissileDirectDamage = 25.0f;  // Mini missiles do less damage
    constexpr float kMissileExplosionMaxRadius = 15.0f;
    constexpr float kMiniMissileExplosionMaxRadius = 8.0f;  // Smaller explosion
    constexpr float kMissileExplosionSpeed = 20.0f;
    constexpr float kMissileExplosionThickness = 3.0f;
    constexpr float kMissileExplosionDuration = 1.5f;
    constexpr float kMissileSize = 0.15f;
    constexpr float kThrusterEmissionRate = 500.0f; // Particles per second
    constexpr float kMiniMissileScale = 0.6f;       // Mini missiles are 60% size
  }

  void MissileSystem::LaunchMissile(const glm::vec3 &position, const glm::vec3 &initialVelocity, Enemy *target, float scale, float damage)
  {
    Missile missile{};
    missile.pos = position;
    missile.vel = initialVelocity;
    missile.target = target;
    missile.hasTarget = (target != nullptr && target->IsAlive());
    if (missile.hasTarget)
    {
      missile.targetPos = target->Position();
    }
    missile.life = missile.maxLife;
    missile.active = true;
    missile.thrusterAccumulator = 0.0f;
    missile.scale = scale;
    missile.damage = damage;

    missiles_.push_back(missile);
  }

  void MissileSystem::LaunchMissiles(const glm::vec3 &leftShoulder, const glm::vec3 &rightShoulder, Enemy *target)
  {
    // Calculate V-shape launch directions
    // Left missile goes left and up, right missile goes right and up
    glm::vec3 playerCenter = (leftShoulder + rightShoulder) * 0.5f;
    playerCenter.y = (leftShoulder.y + rightShoulder.y) * 0.5f; // Use average Y

    // Direction from center to each shoulder (outward)
    glm::vec3 leftDir = glm::normalize(leftShoulder - playerCenter);
    glm::vec3 rightDir = glm::normalize(rightShoulder - playerCenter);

    // Combine outward direction with upward component for V-shape
    // Tighter, faster V-shape launch - missiles quickly arc up then track target
    constexpr float kUpwardComponent = 0.4f;   // Moderate upward climb
    constexpr float kOutwardComponent = 0.25f; // Reduced spread for tighter V-shape
    constexpr float kLaunchSpeed = 20.0f;      // Fast launch speed

    glm::vec3 leftVel = glm::normalize(leftDir * kOutwardComponent + glm::vec3(0.0f, kUpwardComponent, 0.0f)) * kLaunchSpeed;
    glm::vec3 rightVel = glm::normalize(rightDir * kOutwardComponent + glm::vec3(0.0f, kUpwardComponent, 0.0f)) * kLaunchSpeed;

    // Launch the 2 normal missiles
    LaunchMissile(leftShoulder, leftVel, target);
    LaunchMissile(rightShoulder, rightVel, target);

    // If upgraded, launch 2 additional mini missiles with wider spread
    if (upgraded_)
    {
      constexpr float kMiniOutwardComponent = 0.4f; // Wider spread for mini missiles
      constexpr float kMiniUpwardComponent = 0.5f;  // Higher arc for mini missiles
      constexpr float kMiniLaunchSpeed = 18.0f;     // Slightly slower

      glm::vec3 leftMiniVel = glm::normalize(leftDir * kMiniOutwardComponent + glm::vec3(0.0f, kMiniUpwardComponent, 0.0f)) * kMiniLaunchSpeed;
      glm::vec3 rightMiniVel = glm::normalize(rightDir * kMiniOutwardComponent + glm::vec3(0.0f, kMiniUpwardComponent, 0.0f)) * kMiniLaunchSpeed;

      LaunchMissile(leftShoulder, leftMiniVel, target, kMiniMissileScale, kMiniMissileDirectDamage);
      LaunchMissile(rightShoulder, rightMiniVel, target, kMiniMissileScale, kMiniMissileDirectDamage);
    }
  }

  const std::vector<Missile> &MissileSystem::Missiles() const
  {
    return missiles_;
  }

  void MissileSystem::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }

    const float deltaTime = ctx.deltaTime;

    // Play launch sound for newly created missiles
    for (auto &missile : missiles_)
    {
      if (missile.active && !missile.soundHandle_ && params->soundManager)
      {
        missile.soundHandle_ = params->soundManager->PlaySound3D("MISSILE_LAUNCH", missile.pos);
      }
    }

    for (auto &missile : missiles_)
    {
      if (!missile.active)
      {
        continue;
      }

      UpdateMissile(missile, deltaTime, params);
      SpawnThrusterParticles(missile, deltaTime, params);
    }

    // Remove inactive missiles
    missiles_.erase(std::remove_if(missiles_.begin(), missiles_.end(),
                                   [](const Missile &m)
                                   { return !m.active; }),
                    missiles_.end());

    // Update shockwave damage (apply AOE damage from missile explosions)
    if (params && params->shockwaveParticles)
    {
      ApplyShockwaveDamage(params, deltaTime);
    }
  }

  void MissileSystem::ApplyShockwaveDamage(const UpdateParams *params, float deltaTime)
  {
    if (!params || !params->shockwaveParticles || !params->enemies.size())
    {
      return;
    }

    for (auto &wave : *params->shockwaveParticles)
    {
      if (!wave.active || wave.color.r < 0.8f) // Only process missile explosions (orange/red color)
      {
        continue;
      }

      // Apply damage to enemies within shockwave
      for (Enemy *enemy : params->enemies)
      {
        if (!enemy || !enemy->IsAlive())
        {
          continue;
        }

        glm::vec3 enemyPos = enemy->Position();
        glm::vec2 planar(enemyPos.x - wave.center.x, enemyPos.z - wave.center.z);
        float distance = glm::length(planar);

        float inner = glm::max(0.0f, wave.radius - wave.thickness * 0.5f);
        float outer = wave.radius + wave.thickness * 0.5f;

        if (distance >= inner && distance <= outer)
        {
          float dmg = wave.damagePerSecond * deltaTime;
          enemy->ApplyDamage(dmg);
        }
      }
    }
  }

  void MissileSystem::UpdateMissile(Missile &missile, float deltaTime, const UpdateParams *params)
  {
    missile.life -= deltaTime;
    if (missile.life <= 0.0f)
    {
      ExplodeMissile(missile, params);
      return;
    }

    // Update missile sound position
    if (params && params->soundManager && missile.soundHandle_)
    {
      params->soundManager->UpdateSoundPosition(missile.soundHandle_, missile.pos);
    }

    // Update target position if target is still alive
    if (missile.hasTarget && missile.target && missile.target->IsAlive())
    {
      missile.targetPos = missile.target->Position();
    }
    else
    {
      missile.hasTarget = false;
    }

    // Homing behavior: steer toward target
    if (missile.hasTarget)
    {
      glm::vec3 toTarget = missile.targetPos - missile.pos;
      float distToTarget = glm::length(toTarget);

      // Check if we hit the target
      if (distToTarget < kMissileExplosionRadius)
      {
        ExplodeMissile(missile, params);
        return;
      }

      // Calculate desired direction
      glm::vec3 desiredDir = glm::normalize(toTarget);
      glm::vec3 currentDir = glm::normalize(missile.vel);

      // Calculate turn needed
      float dot = glm::dot(currentDir, desiredDir);
      dot = glm::clamp(dot, -1.0f, 1.0f);
      float angle = std::acos(dot);

      // Limit turn rate
      float maxTurn = glm::radians(kMissileMaxTurnRate * deltaTime);
      float turnAmount = glm::min(angle, maxTurn);

      if (turnAmount > 0.001f)
      {
        // Calculate rotation axis
        glm::vec3 axis = glm::cross(currentDir, desiredDir);
        if (glm::length(axis) > 0.001f)
        {
          axis = glm::normalize(axis);

          // Rotate current direction toward desired direction
          glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), turnAmount, axis);
          glm::vec3 newDir = glm::vec3(rotMat * glm::vec4(currentDir, 0.0f));

          // Apply homing strength
          missile.vel = glm::mix(missile.vel, newDir * kMissileSpeed, kMissileHomingStrength * deltaTime);
          missile.vel = glm::normalize(missile.vel) * kMissileSpeed;
        }
      }
    }
    else
    {
      // No target, maintain current velocity
      if (glm::length(missile.vel) < 0.1f)
      {
        missile.vel = glm::vec3(0.0f, -1.0f, 0.0f) * kMissileSpeed; // Fall down
      }
    }

    // Update position
    missile.pos += missile.vel * deltaTime;

    if (params && params->terrainSampler.callback)
    {
      float terrainHeight = params->terrainSampler(missile.pos.x, missile.pos.z);
      if (missile.pos.y <= terrainHeight + 0.35f && missile.vel.y < -1.0f)
      {
        ExplodeMissile(missile, params);
        return;
      }
    }

    // Check collision with enemies
    if (params && params->enemies.size() > 0)
    {
      for (Enemy *enemy : params->enemies)
      {
        if (!enemy || !enemy->IsAlive())
        {
          continue;
        }

        float dist = glm::distance(missile.pos, enemy->Position());
        if (dist < kMissileExplosionRadius + enemy->Radius())
        {
          enemy->ApplyDamage(missile.damage);
          ExplodeMissile(missile, params);
          return;
        }
      }
    }
  }

  void MissileSystem::SpawnThrusterParticles(Missile &missile, float deltaTime, const UpdateParams *params)
  {
    if (!params || !params->thrusterParticles || !missile.active)
    {
      return;
    }

    missile.thrusterAccumulator += kThrusterEmissionRate * deltaTime;
    int numParticles = static_cast<int>(missile.thrusterAccumulator);
    missile.thrusterAccumulator -= static_cast<float>(numParticles);

    if (numParticles <= 0)
    {
      return;
    }

    glm::vec3 backward = -glm::normalize(missile.vel);
    glm::vec3 tailPos = missile.pos + backward * 0.35f;

    auto randFloat = []()
    {
      return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    };
    auto randSigned = [&]()
    {
      return randFloat() * 2.0f - 1.0f;
    };

    constexpr float kParticleLife = 0.45f;
    constexpr float kParticleSpeed = 4.5f;

    auto &particles = *params->thrusterParticles;

    for (int i = 0; i < numParticles; ++i)
    {
      ThrusterParticle particle;
      particle.pos = tailPos + glm::vec3(randSigned() * 0.12f, randSigned() * 0.12f, randSigned() * 0.12f);

      glm::vec3 velDir = backward + glm::vec3(randSigned() * 0.35f, randSigned() * 0.35f, randSigned() * 0.35f);
      velDir = glm::normalize(velDir);
      particle.vel = velDir * kParticleSpeed * (0.85f + randFloat() * 0.5f);

      particle.life = kParticleLife * (0.85f + randFloat() * 0.4f);
      particle.maxLife = particle.life;
      particle.seed = randFloat();
      particle.intensity = 1.2f + randFloat() * 0.6f;
      particle.radiusScale = 0.9f + randFloat() * 0.35f;

      particles.push_back(particle);
    }
  }

  void MissileSystem::SpawnExplosion(const glm::vec3 &position, const UpdateParams *params) const
  {
    if (!params || !params->shockwaveParticles)
    {
      return;
    }

    ShockwaveParticle wave{};
    wave.center = position;
    wave.radius = 0.0f;
    wave.thickness = kMissileExplosionThickness;
    wave.expansionSpeed = kMissileExplosionSpeed;
    wave.maxRadius = kMissileExplosionMaxRadius;
    wave.maxLife = kMissileExplosionDuration;
    wave.life = wave.maxLife;
    wave.damagePerSecond = kMissileExplosionDamage / kMissileExplosionDuration; // Total damage over duration
    wave.color = glm::vec3(1.0f, 0.5f, 0.0f);                                   // Orange/red color for missile explosions
    wave.active = true;

    params->shockwaveParticles->push_back(wave);
  }

  void MissileSystem::ExplodeMissile(Missile &missile, const UpdateParams *params)
  {
    if (params)
    {
      if (params->shockwaveParticles)
      {
        SpawnExplosion(missile.pos, params);
      }
      if (params->soundManager)
      {
        if (missile.soundHandle_)
        {
          params->soundManager->StopSound(missile.soundHandle_);
          missile.soundHandle_ = nullptr;
        }
        params->soundManager->PlaySound3D("MISSILE_EXPLOSION", missile.pos);
      }
    }
    missile.active = false;
  }

  void MissileSystem::Render(const RenderContext &ctx)
  {
    if (missiles_.empty())
    {
      return;
    }

    if (ctx.shadowPass)
    {
      if (!ctx.overrideShader || !missileModel_)
      {
        return;
      }

      for (const auto &missile : missiles_)
      {
        if (!missile.active)
        {
          continue;
        }
        RenderMissileMeshShadow(ctx, missile);
      }
      return;
    }

    if (missileModel_ && missileShader_)
    {
      missileShader_->use();
      missileShader_->setMat4("projection", ctx.projection);
      missileShader_->setMat4("view", ctx.view);
      missileShader_->setMat4("lightSpaceMatrix", ctx.lightSpaceMatrix);
      missileShader_->setVec3("viewPos", ctx.viewPos);
      missileShader_->setVec3("lightPos", ctx.lightPos);
      missileShader_->setVec3("lightIntensity", ctx.lightIntensity);
      missileShader_->setBool("useBaseColor", false);
      missileShader_->setBool("useSSAO", ctx.ssaoEnabled && ctx.ssaoTexture != 0);

      glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
      glBindTexture(GL_TEXTURE_2D, ctx.shadowMapTexture);
      missileShader_->setInt("shadowMap", kShadowMapTextureUnit);

      if (ctx.ssaoEnabled && ctx.ssaoTexture != 0)
      {
        glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
        glBindTexture(GL_TEXTURE_2D, ctx.ssaoTexture);
        missileShader_->setInt("ssaoMap", kSSAOTexUnit);
      }

      for (const auto &missile : missiles_)
      {
        if (!missile.active)
        {
          continue;
        }

        RenderMissileMesh(ctx, missile);
      }
      return;
    }

    if (!shader_ || sphereVAO_ == 0 || sphereIndexCount_ == 0)
    {
      return;
    }

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);

    glBindVertexArray(sphereVAO_);

    for (const auto &m : missiles_)
    {
      if (!m.active)
      {
        continue;
      }

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, m.pos);

      if (glm::length(m.vel) > 0.1f)
      {
        glm::vec3 forward = glm::normalize(m.vel);
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, up));
        up = glm::normalize(glm::cross(right, forward));

        glm::mat4 rotMat(1.0f);
        rotMat[0] = glm::vec4(right, 0.0f);
        rotMat[1] = glm::vec4(up, 0.0f);
        rotMat[2] = glm::vec4(-forward, 0.0f);
        model = model * rotMat;
      }

      model = glm::scale(model, glm::vec3(kMissileSize));
      shader_->setMat4("model", model);
      shader_->setVec4("color", glm::vec4(1.0f, 0.3f, 0.1f, 1.0f));
      glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
  }

  void MissileSystem::RenderMissileMesh(const RenderContext &ctx, const Missile &missile)
  {
    if (!missileShader_ || !missileModel_)
    {
      return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, missile.pos);

    if (glm::length(missile.vel) > 0.1f)
    {
      glm::vec3 forward = glm::normalize(missile.vel);
      glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
      glm::vec3 right = glm::normalize(glm::cross(forward, up));
      up = glm::normalize(glm::cross(right, forward));

      glm::mat4 rotMat(1.0f);
      rotMat[0] = glm::vec4(right, 0.0f);
      rotMat[1] = glm::vec4(up, 0.0f);
      rotMat[2] = glm::vec4(forward, 0.0f);
      model = model * rotMat;
    }

    // Apply per-missile scale (for mini missiles) multiplied by base scale
    model = glm::scale(model, glm::vec3(missileScale_ * missile.scale));
    model = glm::translate(model, -missilePivot_);

    missileShader_->setMat4("model", model);
    missileModel_->Draw(*missileShader_);
  }

  void MissileSystem::RenderMissileMeshShadow(const RenderContext &ctx, const Missile &missile)
  {
    if (!ctx.overrideShader || !missileModel_)
    {
      return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, missile.pos);

    if (glm::length(missile.vel) > 0.1f)
    {
      glm::vec3 forward = glm::normalize(missile.vel);
      glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
      glm::vec3 right = glm::normalize(glm::cross(forward, up));
      up = glm::normalize(glm::cross(right, forward));

      glm::mat4 rotMat(1.0f);
      rotMat[0] = glm::vec4(right, 0.0f);
      rotMat[1] = glm::vec4(up, 0.0f);
      rotMat[2] = glm::vec4(forward, 0.0f);
      model = model * rotMat;
    }

    // Apply per-missile scale (for mini missiles) multiplied by base scale
    model = glm::scale(model, glm::vec3(missileScale_ * missile.scale));
    model = glm::translate(model, -missilePivot_);

    ctx.overrideShader->setMat4("model", model);
    missileModel_->Draw(*ctx.overrideShader);
  }

  void MissileSystem::SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount)
  {
    shader_ = shader;
    sphereVAO_ = sphereVAO;
    sphereIndexCount_ = sphereIndexCount;
  }

  void MissileSystem::SetMissileRenderResources(Shader *shader, Model *model, float scale, const glm::vec3 &pivot)
  {
    missileShader_ = shader;
    missileModel_ = model;
    missileScale_ = scale;
    missilePivot_ = pivot;
  }

} // namespace mecha
