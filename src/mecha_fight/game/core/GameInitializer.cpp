// Windows-specific defines to prevent conflicts - must be before any includes
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include "GameInitializer.h"
#include <glad/glad.h>
#include <learnopengl/filesystem.h>
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include "../rendering/RenderConstants.h"
#include "../systems/MissileSystem.h"

namespace
{
  // Static storage for terrain config pointer used by camera collision
  const mecha::TerrainConfig *g_terrainConfigForCamera = nullptr;

  // Static function that can be used as a function pointer callback
  float TerrainHeightCallback(float x, float z)
  {
    if (g_terrainConfigForCamera)
    {
      static bool loggedSample = false;
      if (!loggedSample)
      {
        std::cout << "[GameInitializer] Terrain sampler active" << std::endl;
        loggedSample = true;
      }
      return mecha::SampleTerrainHeight(x, z, *g_terrainConfigForCamera);
    }
    return 0.0f;
  }
} // anonymous namespace

namespace mecha
{

  GameInitializer::GameInitializer()
  {
  }

  GameInitializer::~GameInitializer()
  {
  }

  GameInitializer::InitializationResult GameInitializer::InitializeWindow(const WindowConfig &config)
  {
    InitializationResult result;

    // Initialize GLFW
    if (!glfwInit())
    {
      result.errorMessage = "Failed to initialize GLFW";
      return result;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Get primary monitor and video mode for fullscreen
    GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);
    if (!mode)
    {
      result.errorMessage = "Failed to get video mode";
      glfwTerminate();
      return result;
    }

    // Create fullscreen window using monitor's resolution
    m_window = glfwCreateWindow(mode->width, mode->height, config.title.c_str(), primaryMonitor, nullptr);
    if (!m_window)
    {
      result.errorMessage = "Failed to create GLFW window";
      glfwTerminate();
      return result;
    }

    glfwMakeContextCurrent(m_window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
      result.errorMessage = "Failed to initialize GLAD";
      glfwTerminate();
      return result;
    }

    // Configure OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << "[GameInitializer] Window and OpenGL context initialized" << std::endl;

    result.success = true;
    result.window = m_window;
    return result;
  }

  bool GameInitializer::LoadResources(ResourceManager &resourceMgr, TerrainConfig &terrainConfig)
  {
    std::cout << "[GameInitializer] Loading game resources..." << std::endl;

    // Initialize resource manager
    if (!resourceMgr.Initialize())
    {
      std::cout << "[GameInitializer] Failed to initialize resource manager" << std::endl;
      return false;
    }

    // Load shaders
    resourceMgr.Shaders().LoadShader("mecha",
                                     FileSystem::getPath("src/mecha_fight/shaders/mecha.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/mecha.fs"));
    resourceMgr.Shaders().LoadShader("terrain",
                                     FileSystem::getPath("src/mecha_fight/shaders/terrain.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/terrain.fs"));
    resourceMgr.Shaders().LoadShader("shadow",
                                     FileSystem::getPath("src/mecha_fight/shaders/shadow.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/shadow.fs"));
    resourceMgr.Shaders().LoadShader("ui",
                                     FileSystem::getPath("src/mecha_fight/shaders/ui.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/ui.fs"));
    resourceMgr.Shaders().LoadShader("color",
                                     FileSystem::getPath("src/mecha_fight/shaders/color.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/color.fs"));
    resourceMgr.Shaders().LoadShader("skybox",
                                     FileSystem::getPath("src/mecha_fight/shaders/skybox.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/skybox.fs"));
    resourceMgr.Shaders().LoadShader("ssao_input",
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao_input.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao_input.fs"));
    resourceMgr.Shaders().LoadShader("ssao",
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao_quad.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao.fs"));
    resourceMgr.Shaders().LoadShader("ssao_blur",
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao_quad.vs"),
                                     FileSystem::getPath("src/mecha_fight/shaders/ssao_blur.fs"));

    const std::string skyboxPath = FileSystem::getPath("resources/textures/skybox/SBS - Cloudy Skyboxes - Cubemap/Cubemap/Cubemap_Sky_05-512x512.png");
    if (!resourceMgr.LoadSkyboxCubemap(skyboxPath))
    {
      std::cerr << "[GameInitializer] Failed to load skybox cubemap: " << skyboxPath << std::endl;
    }

    // Load player mecha model
    Model *mechaModel = resourceMgr.Models().LoadModel("dragon_mecha",
                                                       FileSystem::getPath("resources/objects/new-dragon/new-dragon-mech.gltf"));
    if (!mechaModel)
    {
      std::cout << "[GameInitializer] Failed to load player mecha model" << std::endl;
      return false;
    }

    // Load enemy shark mecha model
    Model *dragonModel = resourceMgr.Models().GetModel("dragon_mecha");
    if (!dragonModel)
    {
      std::cerr << "[GameInitializer] Failed to get dragon_mecha model" << std::endl;
      return false;
    }

    // Load hexapod robot enemy model
    bool hexapodLoaded = resourceMgr.Models().LoadModel("hexapod_robot",
                                                        FileSystem::getPath("resources/objects/episode_71_-_hexapod_robot/scene.gltf"));
    if (!hexapodLoaded)
    {
      std::cerr << "[GameInitializer] Failed to load hexapod_robot model" << std::endl;
      return false;
    }

    // Load energy gun turret model
    bool energyGunLoaded = resourceMgr.Models().LoadModel("energy_gun",
                                                         FileSystem::getPath("resources/objects/energy_gun/scene.gltf"));
    if (!energyGunLoaded)
    {
      std::cerr << "[GameInitializer] Failed to load energy_gun model" << std::endl;
      return false;
    }

    // Load energy gate portal model
    bool energyGateLoaded = resourceMgr.Models().LoadModel("energy_gate",
                                                           FileSystem::getPath("resources/objects/energy_gate_-_classical_style/scene.gltf"));
    if (!energyGateLoaded)
    {
      std::cerr << "[GameInitializer] Failed to load energy_gate model" << std::endl;
      return false;
    }

    bool godzillaLoaded = resourceMgr.Models().LoadModel("mecha_godzilla",
                                                         FileSystem::getPath("resources/objects/deathbringer_from_horizon_zero_dawn/scene.gltf"));
    if (!godzillaLoaded)
    {
      std::cerr << "[GameInitializer] Failed to load deathbringer model" << std::endl;
      return false;
    }

    bool missileLoaded = resourceMgr.Models().LoadModel("r73_missile",
                                                        FileSystem::getPath("resources/objects/r-73_vympel/scene.gltf"));
    if (!missileLoaded)
    {
      std::cerr << "[GameInitializer] Failed to load r-73_vympel missile model" << std::endl;
    }

    // Load icy terrain model and build collision heightfield
    Model *terrainModel = resourceMgr.Models().LoadModel("mountain_range_01",
                                                         FileSystem::getPath("resources/objects/mountain_range_01/scene.gltf"));
    if (!terrainModel)
    {
      std::cerr << "[GameInitializer] Failed to load mountain_range_01 terrain model" << std::endl;
      return false;
    }

    terrainConfig.terrainModel = terrainModel;
    terrainConfig.defaultHeight = terrainConfig.yOffset;

    const auto *terrainInfo = resourceMgr.Models().GetModelInfo("mountain_range_01");
    if (terrainInfo)
    {
      const float maxDimension = std::max(terrainInfo->dimensions.x, terrainInfo->dimensions.z);
      float scaleFactor = 1.0f;
      if (maxDimension > 0.0f)
      {
        scaleFactor = terrainConfig.worldScale / maxDimension;
      }
      terrainConfig.modelScale = glm::vec3(scaleFactor);

      glm::vec3 scaledMin = terrainInfo->boundingMin * terrainConfig.modelScale;
      glm::vec3 scaledMax = terrainInfo->boundingMax * terrainConfig.modelScale;
      glm::vec3 scaledCenter = terrainInfo->center * terrainConfig.modelScale;

      glm::vec3 translation(0.0f);
      translation.x = -scaledCenter.x;
      translation.z = -scaledCenter.z;
      translation.y = terrainConfig.yOffset - scaledMin.y;
      terrainConfig.modelTranslation = translation;

      terrainConfig.boundsMin = scaledMin + translation;
      terrainConfig.boundsMax = scaledMax + translation;

      std::cout << "[GameInitializer] Terrain model scale factor: " << scaleFactor << std::endl;
      std::cout << "[GameInitializer] Terrain bounds min: " << terrainConfig.boundsMin.x << ", " << terrainConfig.boundsMin.y << ", "
                << terrainConfig.boundsMin.z << std::endl;
      std::cout << "[GameInitializer] Terrain bounds max: " << terrainConfig.boundsMax.x << ", " << terrainConfig.boundsMax.y << ", "
                << terrainConfig.boundsMax.z << std::endl;
    }
    else
    {
      float halfSize = terrainConfig.worldScale * 0.5f;
      terrainConfig.boundsMin = glm::vec3(-halfSize, terrainConfig.yOffset, -halfSize);
      terrainConfig.boundsMax = glm::vec3(halfSize, terrainConfig.yOffset + terrainConfig.heightScale, halfSize);
      std::cout << "[GameInitializer] Terrain model info missing, using fallback bounds" << std::endl;
    }

    constexpr int kTerrainHeightSamples = 1024;
    BuildHeightFieldFromModel(*terrainModel, terrainConfig, kTerrainHeightSamples, kTerrainHeightSamples);

    // Generate meshes
    resourceMgr.Meshes().GenerateSphere("enemy_sphere");

    std::cout << "[GameInitializer] All resources loaded successfully" << std::endl;
    return true;
  }

  bool GameInitializer::InitializeDebugSystems(DebugTextRenderer &debugText, unsigned int screenWidth, unsigned int screenHeight)
  {
    const std::string debugFontPath = FileSystem::getPath("resources/fonts/Antonio-Regular.ttf");
    if (!debugText.Init(screenWidth, screenHeight, debugFontPath, 42))
    {
      std::cout << "[GameInitializer] Failed to initialize debug font: " << debugFontPath << std::endl;
      return false;
    }

    std::cout << "[GameInitializer] Debug systems initialized" << std::endl;
    return true;
  }

  bool GameInitializer::InitializeShadowMapper(ShadowMapper &shadowMapper, const TerrainConfig &terrainConfig)
  {
    ShadowMapper::Config shadowConfig;
    shadowConfig.width = kShadowMapResolution;
    shadowConfig.height = kShadowMapResolution;
    const glm::vec3 terrainCenter = 0.5f * (terrainConfig.boundsMin + terrainConfig.boundsMax);
    float horizontalExtent = std::max(terrainConfig.boundsMax.x - terrainConfig.boundsMin.x,
                                      terrainConfig.boundsMax.z - terrainConfig.boundsMin.z) *
                             0.5f;
    if (horizontalExtent <= 0.0f)
    {
      horizontalExtent = terrainConfig.worldScale * 0.5f;
    }

    constexpr float kShadowMargin = 25.0f;
    const float orthoHalf = horizontalExtent + kShadowMargin;
    shadowConfig.orthoLeft = -orthoHalf;
    shadowConfig.orthoRight = orthoHalf;
    shadowConfig.orthoBottom = -orthoHalf;
    shadowConfig.orthoTop = orthoHalf;

    float terrainHeight = terrainConfig.boundsMax.y - terrainConfig.boundsMin.y;
    if (terrainHeight <= 0.0f)
    {
      terrainHeight = terrainConfig.heightScale;
    }
    constexpr float kLightHeightOffset = 50.0f;
    const float lightHeight = terrainHeight + kLightHeightOffset;
    shadowConfig.nearPlane = 0.1f;
    shadowConfig.farPlane = lightHeight + orthoHalf;

    // Keep the light mostly overhead but add a slight horizontal offset so that
    // the light direction is not perfectly aligned with the up-vector; otherwise
    // glm::lookAt would produce an undefined orientation and the depth map would be blank.
    const float horizontalOffset = orthoHalf * 0.35f;
    shadowConfig.lightPosition = terrainCenter + glm::vec3(horizontalOffset, lightHeight, horizontalOffset);
    shadowConfig.target = terrainCenter;
    if (!shadowMapper.Init(shadowConfig))
    {
      std::cout << "[GameInitializer] Failed to initialize shadow mapper" << std::endl;
      return false;
    }

    std::cout << "[GameInitializer] Shadow mapper initialized (extent ~" << orthoHalf * 2.0f << "m)" << std::endl;
    return true;
  }

  void GameInitializer::SetupEntities(GameWorld &world,
                                      MechaPlayer &player,
                                      std::vector<std::shared_ptr<EnemyDrone>> &enemies,
                                      std::vector<std::shared_ptr<TurretEnemy>> &turrets,
                                      std::vector<std::shared_ptr<PortalGate>> &gates,
                                      std::shared_ptr<GodzillaEnemy> &godzillaBoss,
                                      std::shared_ptr<ProjectileSystem> &projectileSystem,
                                      std::shared_ptr<MissileSystem> &missileSystem,
                                      std::shared_ptr<ThrusterParticleSystem> &thrusterSystem,
                                      std::shared_ptr<DashParticleSystem> &dashSystem,
                                      std::shared_ptr<AfterimageParticleSystem> &afterimageSystem,
                                      std::shared_ptr<SparkParticleSystem> &sparkSystem,
                                      std::shared_ptr<ShockwaveParticleSystem> &shockwaveSystem,
                                      std::vector<ThrusterParticle> *thrusterParticles,
                                      std::vector<DashParticle> *dashParticles,
                                      std::vector<AfterimageParticle> *afterimageParticles,
                                      std::vector<SparkParticle> *sparkParticles,
                                      std::vector<ShockwaveParticle> *shockwaveParticles,
                                      ResourceManager &resourceMgr)
  {
    std::cout << "[GameInitializer] Setting up game entities..." << std::endl;

    Shader *mechaShader = resourceMgr.Shaders().GetShader("mecha");
    Shader *colorShader = resourceMgr.Shaders().GetShader("color");
    Model *playerModel = resourceMgr.Models().GetModel("dragon_mecha");
    if (mechaShader && playerModel)
    {
      player.SetRenderResources(mechaShader, playerModel);
    }

    // Set debug render resources for melee hitbox visualization
    const auto *sphereMesh = resourceMgr.Meshes().GetSphere("enemy_sphere");
    if (colorShader && sphereMesh)
    {
      player.SetDebugRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }

    // Add player to world
    auto playerHandle = std::shared_ptr<MechaPlayer>(&player, [](MechaPlayer *) {});
    world.AddEntity(playerHandle);

    // Create and add portal gates
    constexpr int kGateCount = 2;
    gates.clear();
    gates.reserve(kGateCount);

    Model *energyGateModel = resourceMgr.Models().GetModel("energy_gate");
    if (!energyGateModel)
    {
      std::cerr << "[GameInitializer] WARNING: energy_gate model not found! Gates will not be visible." << std::endl;
    }
    else
    {
      std::cout << "[GameInitializer] Found energy_gate model" << std::endl;
    }

    const auto *energyGateInfo = energyGateModel ? resourceMgr.Models().GetModelInfo("energy_gate") : nullptr;
    float gateScale = 1.0f;

    if (energyGateModel && energyGateInfo && energyGateInfo->dimensions.y > 0.0f)
    {
      constexpr float kGateTargetHeight = 8.0f;
      gateScale = kGateTargetHeight / energyGateInfo->dimensions.y;
      std::cout << "[GameInitializer] Configured gate model scale: " << gateScale << std::endl;
    }

    // Position gates at opposite sides of the map (farther apart)
    constexpr float kGateDistance = 120.0f;
    for (int i = 0; i < kGateCount; ++i)
    {
      auto gate = std::make_shared<PortalGate>();

      // Position gates at opposite sides
      float angle = (i * 2.0f * 3.14159f) / kGateCount;
      float x = std::cos(angle) * kGateDistance;
      float z = std::sin(angle) * kGateDistance;
      gate->GetTransform().position = glm::vec3(x, 0.0f, z);
      std::cout << "[GameInitializer] Gate " << i << " spawned at: (" << x << ", 0, " << z << ")" << std::endl;

      if (energyGateModel && energyGateInfo)
      {
        gate->SetModelScale(gateScale);
        gate->SetPivotOffset(energyGateInfo->center);
      }

      if (mechaShader && energyGateModel)
      {
        gate->SetRenderResources(mechaShader, energyGateModel);
        std::cout << "[GameInitializer] Gate " << i << " render resources set" << std::endl;
      }

      gates.push_back(gate);
      world.AddEntity(gate);
    }

    std::cout << "[GameInitializer] Created " << kGateCount << " portal gates" << std::endl;

    // Create and add multiple enemies
    enemies.clear();

    Model *hexapodModel = resourceMgr.Models().GetModel("hexapod_robot");
    const auto *hexapodInfo = hexapodModel ? resourceMgr.Models().GetModelInfo("hexapod_robot") : nullptr;
    float enemyScale = 1.0f;

    if (hexapodModel && hexapodInfo && hexapodInfo->dimensions.y > 0.0f)
      {
      // Scale enemy to bigger size (closer to player size)
      constexpr float kEnemyTargetHeight = 3.5f;
      enemyScale = kEnemyTargetHeight / hexapodInfo->dimensions.y;
      std::cout << "[GameInitializer] Configured enemy model scale: " << enemyScale << std::endl;
    }

    // Spawn enemies around gates
    constexpr float kEnemySpawnRadius = 15.0f;
    constexpr int kMinEnemiesPerGate = 5;
    constexpr int kMaxEnemiesPerGate = 8;
    const int gateCount = static_cast<int>(gates.size());
    std::vector<int> enemiesPerGate(gateCount, 0);
    int totalEnemySpawn = 0;

    for (int gateIdx = 0; gateIdx < gateCount; ++gateIdx)
    {
      int spawnCount = kMinEnemiesPerGate + (std::rand() % (kMaxEnemiesPerGate - kMinEnemiesPerGate + 1));
      enemiesPerGate[gateIdx] = spawnCount;
      totalEnemySpawn += spawnCount;
    }

    enemies.reserve(totalEnemySpawn);

    for (int gateIdx = 0; gateIdx < gateCount; ++gateIdx)
    {
      glm::vec3 gatePos = gates[gateIdx]->Position();
      int spawnCount = enemiesPerGate[gateIdx];
      if (spawnCount <= 0)
      {
        continue;
      }

      for (int i = 0; i < spawnCount; ++i)
      {
        auto enemy = std::make_shared<EnemyDrone>();
        enemy->SetAssociatedGate(gates[gateIdx].get());

        // Position enemies in a circle around the gate
        float angle = (i * 2.0f * 3.14159f) / spawnCount;
        float x = gatePos.x + std::cos(angle) * kEnemySpawnRadius;
        float z = gatePos.z + std::sin(angle) * kEnemySpawnRadius;
        enemy->GetTransform().position = glm::vec3(x, 0.0f, z);
        std::cout << "[GameInitializer] Enemy near Gate " << gateIdx
                  << " spawned at (" << x << ", 0, " << z << ")" << std::endl;

        if (hexapodModel && hexapodInfo)
        {
          enemy->SetModelScale(enemyScale);
        enemy->SetPivotOffset(hexapodInfo->center);
        }

        if (mechaShader && hexapodModel)
        {
          enemy->SetRenderResources(mechaShader, hexapodModel);
        }

        enemies.push_back(enemy);
        world.AddEntity(enemy);
      }
    }

    std::cout << "[GameInitializer] Created " << totalEnemySpawn << " enemies" << std::endl;

    // Create and add turret enemies
    constexpr int kMinTurretsPerGate = 2;
    constexpr int kMaxTurretsPerGate = 3;
    std::vector<int> turretsPerGate(gateCount, 0);
    int totalTurretSpawn = 0;

    for (int gateIdx = 0; gateIdx < gateCount; ++gateIdx)
    {
      int spawnCount = kMinTurretsPerGate + (std::rand() % (kMaxTurretsPerGate - kMinTurretsPerGate + 1));
      turretsPerGate[gateIdx] = spawnCount;
      totalTurretSpawn += spawnCount;
    }

    turrets.clear();
    turrets.reserve(totalTurretSpawn);

    Model *energyGunModel = resourceMgr.Models().GetModel("energy_gun");
    if (!energyGunModel)
    {
      std::cerr << "[GameInitializer] WARNING: energy_gun model not found! Turrets will not be visible." << std::endl;
      }
    else
    {
      std::cout << "[GameInitializer] Found energy_gun model" << std::endl;
    }

    const auto *energyGunInfo = energyGunModel ? resourceMgr.Models().GetModelInfo("energy_gun") : nullptr;
    float turretScale = 1.0f;

    if (energyGunModel && energyGunInfo && energyGunInfo->dimensions.y > 0.0f)
    {
      // Scale turret to reasonable size
      constexpr float kTurretTargetHeight = 5.0f; // Increased from 2.5f to make turret bigger
      turretScale = kTurretTargetHeight / energyGunInfo->dimensions.y;
      std::cout << "[GameInitializer] Configured turret model scale: " << turretScale << std::endl;
      std::cout << "[GameInitializer] Turret model dimensions: " << energyGunInfo->dimensions.x << ", "
                << energyGunInfo->dimensions.y << ", " << energyGunInfo->dimensions.z << std::endl;
    }
    else if (energyGunModel)
    {
      std::cout << "[GameInitializer] Using default turret scale (model info not available)" << std::endl;
      }

    // Spawn turrets around gates
    constexpr float kTurretSpawnRadius = 20.0f;

    for (int gateIdx = 0; gateIdx < gateCount; ++gateIdx)
    {
      glm::vec3 gatePos = gates[gateIdx]->Position();
      int spawnCount = turretsPerGate[gateIdx];
      if (spawnCount <= 0)
      {
        continue;
      }

      for (int i = 0; i < spawnCount; ++i)
      {
        auto turret = std::make_shared<TurretEnemy>();

        // Position turrets in a circle around the gate
        float angle = (i * 2.0f * 3.14159f) / spawnCount;
        float x = gatePos.x + std::cos(angle) * kTurretSpawnRadius;
        float z = gatePos.z + std::sin(angle) * kTurretSpawnRadius;
        turret->GetTransform().position = glm::vec3(x, 0.0f, z);
        std::cout << "[GameInitializer] Turret near Gate " << gateIdx << " spawned at: (" << x << ", 0, " << z << ")" << std::endl;

        if (energyGunModel && energyGunInfo)
        {
          turret->SetModelScale(turretScale);
          turret->SetPivotOffset(energyGunInfo->center);
        }

        if (mechaShader && energyGunModel)
    {
          turret->SetRenderResources(mechaShader, energyGunModel);
          std::cout << "[GameInitializer] Turret near Gate " << gateIdx << " render resources set" << std::endl;
        }
        else
        {
          std::cerr << "[GameInitializer] WARNING: Turret near Gate " << gateIdx << " render resources NOT set! (shader: "
                    << (mechaShader ? "OK" : "NULL") << ", model: " << (energyGunModel ? "OK" : "NULL") << ")" << std::endl;
    }

        // Set laser beam resources
        if (colorShader)
        {
          turret->SetLaserBeamResources(colorShader);
        }

        turrets.push_back(turret);
        world.AddEntity(turret);
      }
    }

    std::cout << "[GameInitializer] Created " << totalTurretSpawn << " turret enemies" << std::endl;

    // Create dormant Godzilla boss
    Model *godzillaModel = resourceMgr.Models().GetModel("mecha_godzilla");
    const auto *godzillaInfo = resourceMgr.Models().GetModelInfo("mecha_godzilla");
    godzillaBoss = std::make_shared<GodzillaEnemy>();
    if (godzillaBoss)
    {
      if (godzillaInfo && godzillaInfo->dimensions.y > 0.0f)
      {
        constexpr float kGodzillaTargetHeight = 45.0f;
        float scale = kGodzillaTargetHeight / godzillaInfo->dimensions.y;
        godzillaBoss->SetModelScale(scale);
        godzillaBoss->SetPivotOffset(godzillaInfo->center);
        std::cout << "[GameInitializer] Godzilla scale configured: " << scale << std::endl;
      }
      if (mechaShader && godzillaModel)
      {
        godzillaBoss->SetRenderResources(mechaShader, godzillaModel);
    }
      godzillaBoss->SetSpawnPosition(glm::vec3(0.0f));
      godzillaBoss->SetShockwaveParticles(shockwaveParticles);
      world.AddEntity(godzillaBoss);
      std::cout << "[GameInitializer] Godzilla entity added (dormant)." << std::endl;
    }

    // Create and add projectile system
    projectileSystem = std::make_shared<ProjectileSystem>();
    // Reuse sphereMesh variable from above
    if (colorShader && sphereMesh)
    {
      projectileSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(projectileSystem);

    // Create and add missile system
    missileSystem = std::make_shared<MissileSystem>();
    if (colorShader && sphereMesh)
    {
      missileSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }

    Model *missileModel = resourceMgr.Models().GetModel("r73_missile");
    if (missileModel && mechaShader)
    {
      float missileScale = 1.0f;
      glm::vec3 missilePivot(0.0f);
      if (const auto *missileInfo = resourceMgr.Models().GetModelInfo("r73_missile"))
      {
        constexpr float kTargetLength = 4.0f;
        float longest = glm::max(glm::max(missileInfo->dimensions.x, missileInfo->dimensions.y), missileInfo->dimensions.z);
        if (longest > 0.0f)
        {
          missileScale = kTargetLength / longest;
        }
        missilePivot = missileInfo->center;
        std::cout << "[GameInitializer] Configured missile model scale: " << missileScale << std::endl;
      }
      missileSystem->SetMissileRenderResources(mechaShader, missileModel, missileScale, missilePivot);
    }

    world.AddEntity(missileSystem);

    // Create and add thruster particle system
    thrusterSystem = std::make_shared<ThrusterParticleSystem>();
    thrusterSystem->SetParticles(thrusterParticles);
    ThrusterParticleSystem::UpdateParams thrusterParams{};
    thrusterParams.gravity = MechaPlayer::kGravity;
    thrusterParams.drag = 3.5f;
    thrusterParams.turbulenceStrength = 14.0f;
    thrusterParams.turbulenceFrequency = 16.0f;
    thrusterParams.upwardDrift = 0.8f;
    thrusterSystem->SetUpdateParams(thrusterParams);
    if (colorShader && sphereMesh)
    {
      thrusterSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(thrusterSystem);

    // Create and add dash particle system
    dashSystem = std::make_shared<DashParticleSystem>();
    dashSystem->SetParticles(dashParticles);
    if (colorShader && sphereMesh)
    {
      dashSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(dashSystem);

    // Create and add dash afterimage system
    afterimageSystem = std::make_shared<AfterimageParticleSystem>();
    afterimageSystem->SetParticles(afterimageParticles);
    if (colorShader && sphereMesh)
    {
      afterimageSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(afterimageSystem);

    // Create and add spark particle system
    sparkSystem = std::make_shared<SparkParticleSystem>();
    sparkSystem->SetParticles(sparkParticles);
    if (colorShader && sphereMesh)
    {
      sparkSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(sparkSystem);

    // Shockwave particle system (used by Godzilla AOE attacks)
    shockwaveSystem = std::make_shared<ShockwaveParticleSystem>();
    shockwaveSystem->SetParticles(shockwaveParticles);
    if (colorShader && sphereMesh)
    {
      shockwaveSystem->SetRenderResources(colorShader, sphereMesh->vao, sphereMesh->indexCount);
    }
    world.AddEntity(shockwaveSystem);

    std::cout << "[GameInitializer] All entities created and added to world" << std::endl;
  }

  void GameInitializer::ConfigurePlayerModel(MechaPlayer &player, ResourceManager &resourceMgr)
  {
    const auto *modelInfo = resourceMgr.Models().GetModelInfo("dragon_mecha");
    if (modelInfo && modelInfo->dimensions.y > 0.0f)
    {
      player.ModelScale() = MechaPlayer::kTargetModelHeight / modelInfo->dimensions.y;
      player.PivotOffset() = modelInfo->center;
      std::cout << "[GameInitializer] Configured player model scale: " << player.ModelScale() << std::endl;
    }
  }

  void GameInitializer::SetupCameraTerrainSampler(ThirdPersonCamera &camera, const TerrainConfig *terrainConfigPtr)
  {
    if (!terrainConfigPtr)
    {
      std::cerr << "[GameInitializer] Warning: terrainConfigPtr is null" << std::endl;
      return;
    }

    // Store the terrain config pointer in the static variable
    g_terrainConfigForCamera = terrainConfigPtr;

    // Set up the terrain sampler with our static function
    ThirdPersonCamera::TerrainSampler terrainSampler;
    terrainSampler.callback = TerrainHeightCallback;
    camera.SetTerrainSampler(terrainSampler);

    std::cout << "[GameInitializer] Camera terrain sampler configured" << std::endl;
  }

} // namespace mecha
