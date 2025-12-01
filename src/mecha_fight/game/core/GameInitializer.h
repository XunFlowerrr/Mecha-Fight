#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>
#include "../entities/MechaPlayer.h"
#include "../entities/EnemyDrone.h"
#include "../entities/TurretEnemy.h"
#include "../entities/PortalGate.h"
#include "../entities/GodzillaEnemy.h"
#include "../systems/ProjectileSystem.h"
#include "../particles/ThrusterParticleSystem.h"
#include "../particles/DashParticleSystem.h"
#include "../particles/AfterimageParticleSystem.h"
#include "../particles/SparkParticleSystem.h"
#include "../particles/ShockwaveParticleSystem.h"
#include "../camera/ThirdPersonCamera.h"
#include "../rendering/ResourceManager.h"
#include "../rendering/ShadowMapper.h"
#include "../ui/DebugTextRenderer.h"
#include "../placeholder/TerrainPlaceholder.h"
#include "../../core/GameWorld.h"

// Forward declarations to avoid including model.h (which has glad issues)
class Model;

namespace mecha
{

  /**
   * @brief Handles complete game initialization sequence
   *
   * Encapsulates:
   * - GLFW setup and window creation
   * - OpenGL context initialization
   * - Resource loading (shaders, models, meshes)
   * - Entity creation and world setup
   * - System initialization (shadow mapper, debug text, etc.)
   */
  class GameInitializer
  {
  public:
    struct WindowConfig
    {
      unsigned int width = 1600;
      unsigned int height = 900;
      std::string title = "Combat Mecha - Arena Battle";
      bool centerWindow = true;
    };

    struct InitializationResult
    {
      bool success = false;
      GLFWwindow *window = nullptr;
      std::string errorMessage;
    };

    GameInitializer();
    ~GameInitializer();

    /**
     * @brief Initialize GLFW, create window, and setup OpenGL context
     */
    InitializationResult InitializeWindow(const WindowConfig &config);

    /**
     * @brief Load all game resources (shaders, models, meshes)
     * @param resourceMgr Resource manager to load into
     * @param terrainConfig Terrain configuration that will be updated with model data
     * @return true if all resources loaded successfully
     */
    bool LoadResources(ResourceManager &resourceMgr, TerrainConfig &terrainConfig);

    /**
     * @brief Initialize debug rendering systems
     * @param debugText Debug text renderer
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     * @return true if initialization successful
     */
    bool InitializeDebugSystems(DebugTextRenderer &debugText, unsigned int screenWidth, unsigned int screenHeight);

    /**
     * @brief Initialize shadow mapping system
     * @param shadowMapper Shadow mapper to initialize
     * @return true if initialization successful
     */
    bool InitializeShadowMapper(ShadowMapper &shadowMapper, const TerrainConfig &terrainConfig);

    /**
     * @brief Create and setup all game entities
     * @param world Game world to add entities to
     * @param player Player entity reference
     * @param enemy Shared pointer to store created enemy
     * @param projectileSystem Shared pointer to store created projectile system
     * @param thrusterSystem Shared pointer to store created thruster system
     * @param dashSystem Shared pointer to store created dash system
     * @param thrusterParticles Vector for thruster particles
     * @param dashParticles Vector for dash particles
     * @param afterimageParticles Vector for dash afterimage particles
     */
    void SetupEntities(GameWorld &world,
                       MechaPlayer &player,
                       std::vector<std::shared_ptr<EnemyDrone>> &enemies,
                       std::vector<std::shared_ptr<TurretEnemy>> &turrets,
                       std::vector<std::shared_ptr<PortalGate>> &gates,
                       std::shared_ptr<class GodzillaEnemy> &godzillaBoss,
                       std::shared_ptr<ProjectileSystem> &projectileSystem,
                       std::shared_ptr<class MissileSystem> &missileSystem,
                       std::shared_ptr<ThrusterParticleSystem> &thrusterSystem,
                       std::shared_ptr<DashParticleSystem> &dashSystem,
                       std::shared_ptr<AfterimageParticleSystem> &afterimageSystem,
                       std::shared_ptr<SparkParticleSystem> &sparkSystem,
                       std::shared_ptr<class ShockwaveParticleSystem> &shockwaveSystem,
                       std::vector<ThrusterParticle> *thrusterParticles,
                       std::vector<DashParticle> *dashParticles,
                       std::vector<AfterimageParticle> *afterimageParticles,
                       std::vector<SparkParticle> *sparkParticles,
                       std::vector<ShockwaveParticle> *shockwaveParticles,
                       ResourceManager &resourceMgr);

    /**
     * @brief Configure player model scale based on loaded model
     * @param player Player to configure
     * @param resourceMgr Resource manager with loaded models
     */
    void ConfigurePlayerModel(MechaPlayer &player, ResourceManager &resourceMgr);

    /**
     * @brief Setup camera terrain collision
     * @param camera Camera to configure
     * @param terrainConfigPtr Pointer to terrain configuration (must remain valid)
     */
    void SetupCameraTerrainSampler(ThirdPersonCamera &camera, const TerrainConfig *terrainConfigPtr);

  private:
    GLFWwindow *m_window = nullptr;
  };

} // namespace mecha
