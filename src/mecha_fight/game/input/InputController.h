#pragma once

#include <memory>
#include "../entities/MechaPlayer.h"
#include "../entities/EnemyDrone.h"
#include "../entities/TurretEnemy.h"
#include "../entities/GodzillaEnemy.h"
#include "../entities/PortalGate.h"
#include "../systems/ProjectileSystem.h"
#include "../systems/MissileSystem.h"
#include "../particles/ThrusterParticleSystem.h"
#include "../particles/DashParticleSystem.h"
#include "../camera/ThirdPersonCamera.h"
#include "../placeholder/TerrainPlaceholder.h"
#include "../ui/DeveloperOverlayUI.h"
#include "../../core/GameWorld.h"
#include <GLFW/glfw3.h>

// Forward declaration
namespace mecha { class SoundManager; }

namespace mecha
{
  class ResourceManager;
}

namespace mecha
{

  /**
   * @brief Handles input processing and entity parameter setup
   *
   * Centralizes input handling logic, parameter configuration for entities,
   * and terrain sampling callbacks. Reduces duplication and coupling in main loop.
   */
  class InputController
  {
  public:
    struct Dependencies
    {
      MechaPlayer *player = nullptr;
      std::vector<std::shared_ptr<EnemyDrone>> enemies;
      std::vector<std::shared_ptr<TurretEnemy>> turrets;
      std::vector<std::shared_ptr<PortalGate>> gates;
      std::shared_ptr<GodzillaEnemy> godzilla;
      std::shared_ptr<ProjectileSystem> projectileSystem;
      std::shared_ptr<MissileSystem> missileSystem;
      std::shared_ptr<ThrusterParticleSystem> thrusterSystem;
      std::shared_ptr<DashParticleSystem> dashSystem;
      ThirdPersonCamera *camera = nullptr;
      GameWorld *world = nullptr;
      DeveloperOverlayState *overlay = nullptr;
      const TerrainConfig *terrainConfig = nullptr;
      ResourceManager *resourceMgr = nullptr;

      // Particle storage
      std::vector<ThrusterParticle> *thrusterParticles = nullptr;
      std::vector<DashParticle> *dashParticles = nullptr;
      std::vector<AfterimageParticle> *afterimageParticles = nullptr;
      std::vector<SparkParticle> *sparkParticles = nullptr;
      std::vector<ShockwaveParticle> *shockwaveParticles = nullptr;

      // Sound system
      class SoundManager *soundManager = nullptr;
    };

    InputController();

    /**
     * @brief Set dependencies for input processing
     */
    void SetDependencies(const Dependencies &deps);

    /**
     * @brief Process input and update entities
     * @param window GLFW window
     * @param deltaTime Frame delta time
     */
    void ProcessInput(GLFWwindow *window, float deltaTime);

  private:
    Dependencies m_deps;

    // Update parameters that must persist for the duration of the frame
    MechaPlayer::UpdateParams m_playerParams;
    EnemyDrone::UpdateParams m_enemyParams;
    TurretEnemy::UpdateParams m_turretParams;
    PortalGate::UpdateParams m_gateParams;
    GodzillaEnemy::UpdateParams m_godzillaParams;
    ProjectileSystem::UpdateParams m_projectileParams;
    MissileSystem::UpdateParams m_missileParams;

    void SetupEntityParameters();
    void UpdateCamera(float deltaTime);
    void ApplyShockwaveRumble(float deltaTime);
    float GetTerrainHeight(float x, float z) const;
  };

} // namespace mecha
