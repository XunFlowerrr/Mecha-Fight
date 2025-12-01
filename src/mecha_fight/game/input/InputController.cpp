#include "InputController.h"
#include "../entities/Enemy.h"
#include "../placeholder/TerrainPlaceholder.h"
#include "../rendering/ResourceManager.h"
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <learnopengl/camera.h>

namespace mecha
{

  InputController::InputController()
  {
  }

  void InputController::SetDependencies(const Dependencies &deps)
  {
    m_deps = deps;
  }

  void InputController::ProcessInput(GLFWwindow *window, float deltaTime)
  {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
      glfwSetWindowShouldClose(window, true);
    }

    SetupEntityParameters();

    UpdateContext ctx{};
    ctx.deltaTime = deltaTime;
    ctx.window = window;

    if (m_deps.world)
    {
      m_deps.world->Update(ctx);
    }

    UpdateCamera(deltaTime);

  }

  void InputController::SetupEntityParameters()
  {

    // Setup player parameters
    if (m_deps.player)
    {
      m_playerParams = MechaPlayer::UpdateParams{};
      m_playerParams.overlay = m_deps.overlay;
      m_playerParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      m_playerParams.thrusterParticles = m_deps.thrusterParticles;
      m_playerParams.dashParticles = m_deps.dashParticles;
      m_playerParams.afterimageParticles = m_deps.afterimageParticles;
      m_playerParams.sparkParticles = m_deps.sparkParticles;
      m_playerParams.shockwaveParticles = m_deps.shockwaveParticles;
      m_playerParams.soundManager = m_deps.soundManager;
      // Set all enemies for melee hit detection (unified vector)
      m_playerParams.enemies.clear();
      size_t extraSlots = m_deps.gates.size();
      if (m_deps.godzilla)
      {
        extraSlots += 1;
      }
      m_playerParams.enemies.reserve(m_deps.enemies.size() + m_deps.turrets.size() + extraSlots);
      // Add EnemyDrones
      for (const auto &enemy : m_deps.enemies)
      {
        if (enemy)
        {
          m_playerParams.enemies.push_back(static_cast<Enemy *>(enemy.get()));
        }
      }
      // Add TurretEnemies
      for (const auto &turret : m_deps.turrets)
      {
        if (turret)
        {
          m_playerParams.enemies.push_back(static_cast<Enemy *>(turret.get()));
        }
      }
      // Add PortalGates
      for (const auto &gate : m_deps.gates)
      {
        if (gate)
        {
          m_playerParams.enemies.push_back(static_cast<Enemy *>(gate.get()));
        }
      }
      if (m_deps.godzilla)
      {
        m_playerParams.enemies.push_back(static_cast<Enemy *>(m_deps.godzilla.get()));
      }
      m_deps.player->SetFramePayload(&m_playerParams);
      bool paused = m_deps.overlay ? m_deps.overlay->animationPaused : false;
      float speed = m_deps.overlay ? m_deps.overlay->animationSpeed : 1.0f;
      m_deps.player->SetAnimationControls(paused, speed);
    }

    // Setup enemy parameters for all enemies
    if (!m_deps.enemies.empty())
    {
      m_enemyParams = EnemyDrone::UpdateParams{};
      m_enemyParams.player = m_deps.player;
      m_enemyParams.projectiles = m_deps.projectileSystem.get();
      m_enemyParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      m_enemyParams.sparkParticles = m_deps.sparkParticles;
      m_enemyParams.soundManager = m_deps.soundManager;

      bool paused = m_deps.overlay ? m_deps.overlay->animationPaused : false;
      // Apply slower animation speed for enemy drones (0.5x multiplier)
      constexpr float kEnemyDroneSpeedMultiplier = 0.25f;
      float speed = m_deps.overlay ? m_deps.overlay->animationSpeed * kEnemyDroneSpeedMultiplier : kEnemyDroneSpeedMultiplier;
      
      for (const auto &enemy : m_deps.enemies)
      {
        if (enemy)
        {
          enemy->SetFramePayload(&m_enemyParams);
          enemy->SetAnimationControls(paused, speed);
        }
      }
    }

    // Setup turret parameters for all turrets
    if (!m_deps.turrets.empty())
    {
      m_turretParams = TurretEnemy::UpdateParams{};
      m_turretParams.player = m_deps.player;
      m_turretParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      m_turretParams.sparkParticles = m_deps.sparkParticles;
      m_turretParams.soundManager = m_deps.soundManager;

      bool paused = m_deps.overlay ? m_deps.overlay->animationPaused : false;
      float speed = m_deps.overlay ? m_deps.overlay->animationSpeed * 1.0f : 1.0f;

      for (const auto &turret : m_deps.turrets)
      {
        if (turret)
        {
          turret->SetFramePayload(&m_turretParams);
          turret->SetAnimationControls(paused, speed);
        }
      }
    }

    // Setup gate parameters for all gates
    if (!m_deps.gates.empty())
    {
      m_gateParams = PortalGate::UpdateParams{};
      m_gateParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      m_gateParams.sparkParticles = m_deps.sparkParticles;
      m_gateParams.soundManager = m_deps.soundManager;

      for (const auto &gate : m_deps.gates)
      {
        if (gate)
        {
          gate->SetFramePayload(&m_gateParams);
        }
      }
    }

    // Setup Godzilla parameters if present
    if (m_deps.godzilla)
    {
      m_godzillaParams = GodzillaEnemy::UpdateParams{};
      m_godzillaParams.player = m_deps.player;
      m_godzillaParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      m_godzillaParams.shockwaveParticles = m_deps.shockwaveParticles;
      m_godzillaParams.thrusterParticles = m_deps.thrusterParticles;
      m_godzillaParams.projectiles = m_deps.projectileSystem.get();
      m_godzillaParams.soundManager = m_deps.soundManager;
      m_deps.godzilla->SetFramePayload(&m_godzillaParams);
    }

    // Setup projectile system parameters
    if (m_deps.projectileSystem)
    {
      m_projectileParams = ProjectileSystem::UpdateParams{};
      m_projectileParams.player = m_deps.player;
      m_projectileParams.enemies.clear();
      size_t projectileReserve = m_deps.enemies.size() + m_deps.turrets.size() + m_deps.gates.size();
      if (m_deps.godzilla)
      {
        projectileReserve += 1;
      }
      m_projectileParams.enemies.reserve(projectileReserve);
      // Add EnemyDrones
      for (const auto &enemy : m_deps.enemies)
      {
        if (enemy)
        {
          m_projectileParams.enemies.push_back(static_cast<Enemy *>(enemy.get()));
        }
      }
      // Add TurretEnemies
      for (const auto &turret : m_deps.turrets)
      {
        if (turret)
        {
          m_projectileParams.enemies.push_back(static_cast<Enemy *>(turret.get()));
        }
      }
      // Add PortalGates
      for (const auto &gate : m_deps.gates)
      {
        if (gate)
        {
          m_projectileParams.enemies.push_back(static_cast<Enemy *>(gate.get()));
        }
      }
      if (m_deps.godzilla)
      {
        m_projectileParams.enemies.push_back(static_cast<Enemy *>(m_deps.godzilla.get()));
      }
      m_projectileParams.overlay = m_deps.overlay;
      m_projectileParams.soundManager = m_deps.soundManager;
      m_deps.projectileSystem->SetFramePayload(&m_projectileParams);
    }

    // Setup missile system parameters
    if (m_deps.missileSystem)
    {
      m_missileParams = MissileSystem::UpdateParams{};
      m_missileParams.player = m_deps.player;
      m_missileParams.thrusterParticles = m_deps.thrusterParticles;
      m_missileParams.shockwaveParticles = m_deps.shockwaveParticles;
      m_missileParams.terrainSampler.callback = [this](float x, float z)
      { return GetTerrainHeight(x, z); };
      // Add all enemies
      m_missileParams.enemies.clear();
      m_missileParams.enemies.reserve(m_deps.enemies.size() + m_deps.turrets.size() + m_deps.gates.size() + (m_deps.godzilla ? 1 : 0));
      for (const auto &enemy : m_deps.enemies)
      {
        if (enemy)
        {
          m_missileParams.enemies.push_back(static_cast<Enemy *>(enemy.get()));
    }
      }
      for (const auto &turret : m_deps.turrets)
      {
        if (turret)
        {
          m_missileParams.enemies.push_back(static_cast<Enemy *>(turret.get()));
        }
      }
      for (const auto &gate : m_deps.gates)
      {
        if (gate)
        {
          m_missileParams.enemies.push_back(static_cast<Enemy *>(gate.get()));
        }
      }
      if (m_deps.godzilla)
      {
        m_missileParams.enemies.push_back(static_cast<Enemy *>(m_deps.godzilla.get()));
      }
      m_missileParams.soundManager = m_deps.soundManager;
      m_deps.missileSystem->SetFramePayload(&m_missileParams);
    }

  }

  void InputController::UpdateCamera(float deltaTime)
  {
    if (!m_deps.camera || !m_deps.player || !m_deps.overlay)
    {
      return;
    }

    // Update camera to follow mecha
    MovementState &movement = m_deps.player->Movement();
    float camDistance = glm::clamp(m_deps.overlay->cameraDistance, 3.0f, 12.0f);
    m_deps.camera->Update(movement.position, camDistance, MechaPlayer::kCameraHeightOffset);

    // Apply camera rumble from nearby shockwaves
    ApplyShockwaveRumble(deltaTime);

    // Align mecha forward direction with camera direction for aiming
    movement.yawDegrees = m_deps.camera->GetYaw() + 180.0f;
  }

  void InputController::ApplyShockwaveRumble(float deltaTime)
  {
    if (!m_deps.camera || !m_deps.player || !m_deps.shockwaveParticles)
    {
      return;
    }

    static float rumbleTime = 0.0f;
    static float lingeringRumbleIntensity = 0.0f; // Lingering effect that fades out
    rumbleTime += deltaTime;
    if (rumbleTime > 1000.0f) // Prevent overflow
    {
      rumbleTime = std::fmod(rumbleTime, 1000.0f);
    }

    glm::vec3 playerPos = m_deps.player->Movement().position;
    float currentRumbleIntensity = 0.0f;

    // Check all active shockwaves for proximity
    for (const auto &wave : *m_deps.shockwaveParticles)
    {
      if (!wave.active)
      {
        continue;
      }

      // Calculate distance from player to shockwave center (planar distance)
      glm::vec2 player2D(playerPos.x, playerPos.z);
      glm::vec2 wave2D(wave.center.x, wave.center.z);
      float distance = glm::length(player2D - wave2D);

      // Calculate if player is within the shockwave ring
      float inner = glm::max(0.0f, wave.radius - wave.thickness * 0.5f);
      float outer = wave.radius + wave.thickness * 0.5f;

      if (distance >= inner && distance <= outer)
      {
        // Calculate rumble intensity based on proximity to shockwave center
        // Stronger when closer to the center of the ring
        float normalizedDist = (distance - inner) / (outer - inner + 0.001f);
        float intensity = 1.0f - normalizedDist; // 1.0 at inner edge, 0.0 at outer edge
        intensity = glm::clamp(intensity, 0.0f, 1.0f);

        // Scale intensity by shockwave size (bigger shockwaves = more rumble)
        float sizeFactor = glm::min(1.0f, wave.radius / 50.0f);
        intensity *= sizeFactor;

        currentRumbleIntensity = glm::max(currentRumbleIntensity, intensity);
      }
    }

    // Update lingering rumble intensity (accumulate and fade out)
    constexpr float kLingeringFadeRate = 0.3f; // Slower fade rate for longer lingering effect (was 0.8f)
    constexpr float kLingeringAccumulation = 3.0f; // Increased accumulation rate (was 1.5f)
    
    if (currentRumbleIntensity > 0.01f)
    {
      // Accumulate intensity when hit by shockwave
      lingeringRumbleIntensity = glm::min(1.0f, lingeringRumbleIntensity + currentRumbleIntensity * kLingeringAccumulation * deltaTime);
    }
    else
    {
      // Fade out lingering effect (slower fade = longer lingering)
      lingeringRumbleIntensity = glm::max(0.0f, lingeringRumbleIntensity - kLingeringFadeRate * deltaTime);
    }

    // Use the maximum of current and lingering intensity
    float finalRumbleIntensity = glm::max(currentRumbleIntensity, lingeringRumbleIntensity);

    // Apply rumble offset to camera position
    if (finalRumbleIntensity > 0.01f)
    {
      constexpr float kRumbleStrength = 1.2f; // Much more intense rumble (was 0.5f)
      constexpr float kRumbleFrequency = 30.0f; // Higher frequency for more noticeable effect (was 25.0f)

      // Generate random rumble offset using time-based noise
      float rumbleX = (std::sin(rumbleTime * kRumbleFrequency) + std::cos(rumbleTime * kRumbleFrequency * 1.3f)) * 0.5f;
      float rumbleY = (std::sin(rumbleTime * kRumbleFrequency * 0.7f) + std::cos(rumbleTime * kRumbleFrequency * 1.1f)) * 0.5f;
      float rumbleZ = (std::sin(rumbleTime * kRumbleFrequency * 0.9f) + std::cos(rumbleTime * kRumbleFrequency * 1.5f)) * 0.5f;

      glm::vec3 rumbleOffset(
        rumbleX * kRumbleStrength * finalRumbleIntensity,
        rumbleY * kRumbleStrength * finalRumbleIntensity,
        rumbleZ * kRumbleStrength * finalRumbleIntensity
      );

      // Apply rumble to camera position
      Camera &camera = m_deps.camera->GetCamera();
      camera.Position += rumbleOffset;
    }
  }

  float InputController::GetTerrainHeight(float x, float z) const
  {
    if (m_deps.terrainConfig)
    {
      return SampleTerrainHeight(x, z, *m_deps.terrainConfig);
    }
    return 0.0f;
  }

} // namespace mecha
