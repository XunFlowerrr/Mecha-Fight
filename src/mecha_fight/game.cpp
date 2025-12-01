// Windows-specific defines to prevent conflicts - must be before any includes
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
// render
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <learnopengl/filesystem.h>

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

#include "core/GameWorld.h"
#include "game/entities/EnemyDrone.h"
#include "game/entities/TurretEnemy.h"
#include "game/entities/GodzillaEnemy.h"
#include "game/entities/Enemy.h"
#include "game/GameplayTypes.h"
#include "game/entities/MechaPlayer.h"
#include "game/systems/ProjectileSystem.h"
#include "game/systems/MissileSystem.h"
#include "game/ui/DebugTextRenderer.h"
#include "game/ui/DeveloperOverlayUI.h"
#include "game/ui/HudRenderer.h"
#include "game/particles/AfterimageParticleSystem.h"
#include "game/particles/DashParticleSystem.h"
#include "game/particles/ThrusterParticleSystem.h"
#include "game/particles/ShockwaveParticleSystem.h"
#include "game/placeholder/EnemyPlaceholder.h"
#include "game/placeholder/TerrainPlaceholder.h"
#include "game/camera/ThirdPersonCamera.h"
#include "game/rendering/ShadowMapper.h"
#include "game/rendering/ResourceManager.h"
#include "game/rendering/SceneRenderer.h"
#include "game/input/InputController.h"
#include "game/ui/GameHUD.h"
#include "game/core/GameInitializer.h"
#include "game/audio/ISoundController.h"
#include "game/audio/MiniaudioSoundController.h"
#include "game/audio/ProximitySoundSystem.h"
#include "game/audio/SoundManager.h"
#include "game/audio/SoundRegistry.h"
#include "game/audio/BackgroundMusicSystem.h"
#include "game/ui/MainMenu.h"
#include "game/ui/GameOverScreen.h"
#include "game/systems/ObjectiveSystem.h"

using mecha::AfterimageParticle;
using mecha::AfterimageParticleSystem;
using mecha::BoostState;
using mecha::CombatState;
using mecha::DashParticle;
using mecha::DashParticleSystem;
using mecha::DeveloperOverlayState;
using mecha::EnemyDrone;
using mecha::FlightState;
using mecha::GameHUD;
using mecha::GameInitializer;
using mecha::GameWorld;
using mecha::GodzillaEnemy;
using mecha::HudRenderer;
using mecha::InputController;
using mecha::ISoundController;
using mecha::MainMenu;
using mecha::MechaPlayer;
using mecha::MiniaudioSoundController;
using mecha::MissileSystem;
using mecha::MovementState;
using mecha::ProjectileSystem;
using mecha::ProximitySoundSystem;
using mecha::RenderContext;
using mecha::ResourceManager;
using mecha::SceneRenderer;
using mecha::ShadowMapper;
using mecha::ShockwaveParticle;
using mecha::ShockwaveParticleSystem;
using mecha::SoundManager;
using mecha::ThrusterParticle;
using mecha::ThrusterParticleSystem;
using mecha::UpdateContext;

// Game state enum
enum class GameState
{
    MainMenu,
    Playing,
    Paused,
    PlayerDeath,
    Victory
};

constexpr float kInitialMasterVolume = 1.0f; // Adjust this to change the game's default audio loudness

// settings
unsigned int SCR_WIDTH = 1600;
unsigned int SCR_HEIGHT = 900;

static mecha::DebugTextRenderer gDebugText;
static DeveloperOverlayState gDevOverlay;
[[maybe_unused]] static bool gDevOverlayVolumeInitialized = []()
{
    gDevOverlay.masterVolume = kInitialMasterVolume;
    return true;
}();
static mecha::DeveloperOverlayUI gDevOverlayUI(gDevOverlay, gDebugText);
static bool gCursorCaptured = true;

static MechaPlayer gMecha;
static GameWorld gWorld;
static std::vector<std::shared_ptr<EnemyDrone>> gEnemies;
static std::vector<std::shared_ptr<mecha::TurretEnemy>> gTurrets;
static std::vector<std::shared_ptr<mecha::PortalGate>> gGates;
static std::shared_ptr<GodzillaEnemy> gGodzilla;
static std::shared_ptr<ProjectileSystem> gProjectileSystem;
static std::shared_ptr<MissileSystem> gMissileSystem;
static std::shared_ptr<ThrusterParticleSystem> gThrusterParticleSystem;
static std::shared_ptr<AfterimageParticleSystem> gDashAfterimageSystem;
static std::shared_ptr<DashParticleSystem> gDashParticleSystem;
static std::shared_ptr<mecha::SparkParticleSystem> gSparkParticleSystem;
static std::shared_ptr<ShockwaveParticleSystem> gShockwaveSystem;
static HudRenderer gHudRenderer;
static GameHUD gGameHUD;
static InputController gInputController;
static SceneRenderer gSceneRenderer;
static MainMenu gMainMenu;
static mecha::GameOverScreen gGameOverScreen;
static GameState gGameState = GameState::MainMenu;
static mecha::ObjectiveSystem gObjectiveSystem;
static std::vector<bool> gPreviousPortalStates; // For tracking portal destruction events
static bool gPreviousBossAlive = true;          // For tracking boss defeat event
static float gBossDeathTimer = 0.0f;            // Timer for victory delay after boss death

static void SetCursorCapture(GLFWwindow *window, bool capture);

static mecha::ThirdPersonCamera gCamera;

// Sound system
static std::unique_ptr<MiniaudioSoundController> gSoundController;
static std::unique_ptr<SoundManager> gSoundManager;
static std::shared_ptr<ProximitySoundSystem> gProximitySoundSystem;
static std::unique_ptr<mecha::BackgroundMusicSystem> gBackgroundMusic;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
static bool gGodzillaSpawned = false;

static void SetCursorCapture(GLFWwindow *window, bool capture)
{
    gCursorCaptured = capture;
    glfwSetInputMode(window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (capture)
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        float centerX = width * 0.5f;
        float centerY = height * 0.5f;
        gCamera.ResetMouseTracking(centerX, centerY);
        glfwSetCursorPos(window, static_cast<double>(centerX), static_cast<double>(centerY));
    }
}

// Thruster particles
std::vector<ThrusterParticle> thrusterParticles;

// Dash particles (energy burst)
std::vector<DashParticle> dashParticles;
std::vector<AfterimageParticle> dashAfterimageParticles;
std::vector<mecha::SparkParticle> sparkParticles;
std::vector<ShockwaveParticle> shockwaveParticles;

// UI constants
const float FOCUS_CIRCLE_RADIUS = 120.0f; // pixels, circle in center of screen for targeting UI

// Terrain configuration (will be replaced with model later)
static mecha::TerrainConfig gTerrainConfig;

// Shadow mapping
static ShadowMapper gShadowMapper;

// Resource manager (factories for shaders, models, meshes)
static ResourceManager gResourceManager;

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    gDebugText.Resize(width, height);
    gMainMenu.Resize(width, height);
}

// glfw: whenever the mouse moves, this callback is called
// Third-person shooter style: mouse rotates camera around the mecha
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    static float lastX = 0.0f;
    static float lastY = 0.0f;
    static bool firstMouse = true;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (!gCursorCaptured)
    {
        return;
    }

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    gCamera.GetCamera().ProcessMouseScroll(static_cast<float>(yoffset));
}

int main()
{
    // Initialize game using GameInitializer
    GameInitializer initializer;

    GameInitializer::WindowConfig windowConfig;
    windowConfig.width = SCR_WIDTH;
    windowConfig.height = SCR_HEIGHT;
    windowConfig.title = "Combat Mecha - Arena Battle";
    windowConfig.centerWindow = true;

    auto initResult = initializer.InitializeWindow(windowConfig);
    if (!initResult.success)
    {
        std::cout << "Initialization failed: " << initResult.errorMessage << std::endl;
        return -1;
    }

    GLFWwindow *window = initResult.window;

    // Get actual framebuffer size (will be fullscreen resolution, accounts for high-DPI displays)
    int actualWidth, actualHeight;
    glfwGetFramebufferSize(window, &actualWidth, &actualHeight);
    SCR_WIDTH = static_cast<unsigned int>(actualWidth);
    SCR_HEIGHT = static_cast<unsigned int>(actualHeight);

    // Setup callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    SetCursorCapture(window, true);

    // Initialize debug systems
    if (!initializer.InitializeDebugSystems(gDebugText, SCR_WIDTH, SCR_HEIGHT))
    {
        glfwTerminate();
        return -1;
    }

    // Load all resources
    if (!initializer.LoadResources(gResourceManager, gTerrainConfig))
    {
        glfwTerminate();
        return -1;
    }

    // Configure player model
    initializer.ConfigurePlayerModel(gMecha, gResourceManager);

    Model *mechaModel = gResourceManager.Models().GetModel("dragon_mecha");
    if (mechaModel)
    {
        gDevOverlayUI.Reset(*mechaModel);
    }

    // Setup all entities
    initializer.SetupEntities(gWorld, gMecha, gEnemies, gTurrets, gGates, gGodzilla, gProjectileSystem,
                              gMissileSystem, gThrusterParticleSystem, gDashParticleSystem, gDashAfterimageSystem,
                              gSparkParticleSystem, gShockwaveSystem,
                              &thrusterParticles, &dashParticles, &dashAfterimageParticles, &sparkParticles,
                              &shockwaveParticles, gResourceManager);

    // Initialize objective system with total portal count
    gObjectiveSystem.Initialize(static_cast<int>(gGates.size()));

    // Initialize portal state tracking for objective detection
    gPreviousPortalStates.clear();
    gPreviousPortalStates.reserve(gGates.size());
    for (const auto &gate : gGates)
    {
        gPreviousPortalStates.push_back(gate ? gate->IsAlive() : false);
    }
    gPreviousBossAlive = gGodzilla ? gGodzilla->IsAlive() : true;

    // Initialize sound system (using miniaudio for cross-platform support including macOS arm64)
    gSoundController = std::make_unique<MiniaudioSoundController>();
    if (gSoundController)
    {
        // Create sound manager
        gSoundManager = std::make_unique<SoundManager>(gSoundController.get());
        gSoundManager->SetMasterVolume(gDevOverlay.masterVolume);

        // Get proximity system from manager and add to world
        gProximitySoundSystem = gSoundManager->GetProximitySystem();
        if (gProximitySoundSystem)
        {
            gWorld.AddEntity(gProximitySoundSystem);
        }

        // Register all sounds with the manager
        gSoundManager->RegisterSound("PLAYER_SHOOT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_SHOOT, 0.7f, 100.0f, false, 0.0f});
        gSoundManager->RegisterSound("PLAYER_DASH",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_DASH, 0.2f, 60.0f, false, 0.0f});
        gSoundManager->RegisterSound("PLAYER_MELEE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_MELEE, 1.0f, 50.0f, false, 0.0f});
        gSoundManager->RegisterSound("PLAYER_MELEE_CONTINUE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_MELEE_CONTINUE, 0.8f, 50.0f, true, 0.0f});
        gSoundManager->RegisterSound("PLAYER_DAMAGE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_DAMAGE, 0.8f, 40.0f, false, 0.25f});
        gSoundManager->RegisterSound("PLAYER_FLIGHT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_FLIGHT, 0.9f, 80.0f, true, 0.0f});
        gSoundManager->RegisterSound("PLAYER_WALKING",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_WALKING, 1.5f, 40.0f, true, 0.0f});
        gSoundManager->RegisterSound("PLAYER_LASER",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PLAYER_LASER, 0.7f, 60.0f, true, 0.0f});

        gSoundManager->RegisterSound("ENEMY_SHOOT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::ENEMY_SHOOT, 0.6f, 50.0f, false, 0.05f});
        gSoundManager->RegisterSound("ENEMY_DEATH",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::ENEMY_DEATH, 0.8f, 60.0f, false, 0.1f});
        gSoundManager->RegisterSound("ENEMY_DRONE_MOVEMENT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::ENEMY_DRONE_MOVEMENT, 0.05f, 80.0f, true, 0.0f});
        gSoundManager->RegisterSound("ENEMY_TURRET_LASER",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::ENEMY_TURRET_LASER, 0.7f, 200.0f, true, 0.0f});

        gSoundManager->RegisterSound("PROJECTILE_IMPACT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::PROJECTILE_IMPACT, 0.7f, 40.0f, false, 0.05f});
        gSoundManager->RegisterSound("MISSILE_LAUNCH",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::MISSILE_LAUNCH, 0.8f, 100.0f, true, 0.0f});
        gSoundManager->RegisterSound("MISSILE_EXPLOSION",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::MISSILE_EXPLOSION, 10.0f, 3000.0f, false, 0.2f});

        gSoundManager->RegisterSound("BOSS_DEATH",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::BOSS_DEATH, 3.0f, 7500.0f, false, 0.0f});
        gSoundManager->RegisterSound("BOSS_MOVEMENT",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::BOSS_MOVEMENT, 2.0f, 3000.0f, true, 0.0f});
        gSoundManager->RegisterSound("BOSS_PROJECTILE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::BOSS_PROJECTILE, 0.4f, 220.0f, false, 0.05f});
        gSoundManager->RegisterSound("BOSS_SHOCKWAVE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::BOSS_SHOCKWAVE, 2.5f, 500.0f, false, 0.0f});

        gSoundManager->RegisterSound("GATE_COLLAPSE",
                                     SoundManager::SoundConfig{mecha::SoundRegistry::GATE_COLLAPSE, 1.0f, 120.0f, false, 0.0f});

        // Preload all registered sounds
        for (const auto &[soundName, config] : gSoundManager->GetRegisteredSounds())
        {
            gSoundManager->PreloadSound(soundName);
        }

        // Register with ResourceManager for unified access
        gResourceManager.SetSoundManager(gSoundManager.get());
        gResourceManager.SetSoundController(gSoundController.get()); // Keep for legacy access

        // Initialize background music system
        gBackgroundMusic = std::make_unique<mecha::BackgroundMusicSystem>(gSoundController.get());
        gBackgroundMusic->Initialize();
        gBackgroundMusic->SetVolume(0.4f); // Set music volume (lower than sound effects)

        std::cout << "[Game] Sound system initialized" << std::endl;
    }
    else
    {
        std::cerr << "[Game] Warning: Sound system failed to initialize" << std::endl;
    }

    // Setup input controller dependencies (after all entities are created)
    InputController::Dependencies inputDeps;
    inputDeps.player = &gMecha;
    inputDeps.enemies = gEnemies;
    inputDeps.turrets = gTurrets;
    inputDeps.gates = gGates;
    inputDeps.godzilla = gGodzilla;
    inputDeps.projectileSystem = gProjectileSystem;
    inputDeps.missileSystem = gMissileSystem;
    inputDeps.thrusterSystem = gThrusterParticleSystem;
    inputDeps.dashSystem = gDashParticleSystem;
    inputDeps.camera = &gCamera;
    inputDeps.world = &gWorld;
    inputDeps.overlay = &gDevOverlay;
    inputDeps.terrainConfig = &gTerrainConfig;
    inputDeps.resourceMgr = &gResourceManager;
    inputDeps.thrusterParticles = &thrusterParticles;
    inputDeps.dashParticles = &dashParticles;
    inputDeps.afterimageParticles = &dashAfterimageParticles;
    inputDeps.sparkParticles = &sparkParticles;
    inputDeps.shockwaveParticles = &shockwaveParticles;
    inputDeps.soundManager = gSoundManager.get();
    gInputController.SetDependencies(inputDeps);

    // Setup camera terrain sampler
    initializer.SetupCameraTerrainSampler(gCamera, &gTerrainConfig);

    // Initialize shadow mapper
    if (!initializer.InitializeShadowMapper(gShadowMapper, gTerrainConfig))
    {
        glfwTerminate();
        return -1;
    }

    // Initialize scene renderer
    constexpr float kSceneFarPlane = 600.0f;
    SceneRenderer::RenderConfig renderConfig;
    renderConfig.screenWidth = SCR_WIDTH;
    renderConfig.screenHeight = SCR_HEIGHT;
    renderConfig.nearPlane = gCamera.GetNearPlane();
    renderConfig.farPlane = kSceneFarPlane;
    renderConfig.clearColor = glm::vec3(0.05f, 0.05f, 0.05f);
    renderConfig.lightIntensity = glm::vec3(1.3f, 1.25f, 1.2f);
    renderConfig.showLightDebug = true;
    renderConfig.lightMarkerScale = 5.0f;
    renderConfig.enableSSAO = true;
    renderConfig.ssaoRadius = 0.95f;
    renderConfig.ssaoBias = 0.025f;
    renderConfig.ssaoPower = 0.85f;
    renderConfig.ssaoStrength = 0.25f;
    renderConfig.enableSkybox = true;
    renderConfig.skyboxIntensity = 1.0f;
    renderConfig.skyboxTint = glm::vec3(1.0f);

    if (!gSceneRenderer.Initialize(renderConfig))
    {
        glfwTerminate();
        return -1;
    }

    gSceneRenderer.SetDependencies(&gResourceManager, &gShadowMapper, &gWorld);

    // Initialize main menu
    const std::string menuBackgroundPath = FileSystem::getPath("resources/images/main-menu.png");
    if (!gMainMenu.Initialize(SCR_WIDTH, SCR_HEIGHT, menuBackgroundPath))
    {
        std::cerr << "[Game] Warning: Main menu initialization failed, continuing without background" << std::endl;
    }

    // Initialize game over screen
    if (!gGameOverScreen.Initialize(SCR_WIDTH, SCR_HEIGHT))
    {
        std::cerr << "[Game] Warning: Game over screen initialization failed" << std::endl;
    }

    gGameState = GameState::MainMenu;
    SetCursorCapture(window, false); // Show cursor for menu

    // Seed random number generator
    std::srand((unsigned)std::time(nullptr));

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float frameDelta = currentFrame - lastFrame;
        lastFrame = currentFrame;
        deltaTime = frameDelta * gDevOverlay.timeScale;

        // Handle main menu state
        if (gGameState == GameState::MainMenu)
        {
            // Process menu input
            gMainMenu.ProcessInput(window);

            // Check menu state transitions
            if (gMainMenu.GetState() == MainMenu::MenuState::StartGame)
            {
                gGameState = GameState::Playing;
                SetCursorCapture(window, true); // Capture cursor for gameplay
                gMainMenu.Reset();

                // Start normal background music when game starts
                if (gBackgroundMusic)
                {
                    gBackgroundMusic->SetStage(mecha::BackgroundMusicSystem::MusicStage::Normal, 2.0f);
                }

                std::cout << "[Game] Starting game..." << std::endl;
            }
            else if (gMainMenu.GetState() == MainMenu::MenuState::Quit)
            {
                glfwSetWindowShouldClose(window, true);
            }

            // Render menu
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            Shader *uiShader = gResourceManager.Shaders().GetShader("ui");
            if (uiShader)
            {
                gMainMenu.Render(*uiShader, gResourceManager.GetUIQuadVAO());
            }

            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue; // Skip game logic while in menu
        }

        // Handle player death or victory screen
        if (gGameState == GameState::PlayerDeath || gGameState == GameState::Victory)
        {
            // Process game over screen input
            gGameOverScreen.ProcessInput(window);
            gGameOverScreen.Update(deltaTime);

            // Check for state transitions
            auto result = gGameOverScreen.GetAndClearResult();
            if (result != mecha::GameOverScreen::SelectionResult::None)
            {
                switch (result)
                {
                case mecha::GameOverScreen::SelectionResult::Continue:
                    gMecha.ResetHealth();
                    gGameState = GameState::Playing;
                    SetCursorCapture(window, true);
                    break;
                case mecha::GameOverScreen::SelectionResult::GodMode:
                    gMecha.ResetHealth();
                    gMecha.SetGodMode(true);
                    gGameState = GameState::Playing;
                    SetCursorCapture(window, true);
                    break;
                case mecha::GameOverScreen::SelectionResult::ReturnToMenu:
                    gGameState = GameState::MainMenu;
                    // Note: A full game reset would be needed here for production
                    break;
                default:
                    break;
                }
                gGameOverScreen.Hide();
            }

            // Render the scene in the background (frozen)
            SceneRenderer::FrameData bgFrameData{};
            bgFrameData.projection = glm::perspective(glm::radians(gCamera.GetCamera().Zoom),
                                                      (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                      gCamera.GetNearPlane(), kSceneFarPlane);
            bgFrameData.view = gCamera.GetCamera().GetViewMatrix();
            bgFrameData.viewPos = gCamera.GetCamera().Position;
            bgFrameData.mechaPosition = gMecha.Movement().position;
            bgFrameData.mechaYawDegrees = gMecha.Movement().yawDegrees;
            bgFrameData.mechaPitchDegrees = gMecha.Movement().pitchDegrees;
            bgFrameData.mechaRollDegrees = gMecha.Movement().rollDegrees;
            bgFrameData.mechaModelScale = gMecha.ModelScale();
            bgFrameData.mechaPivotOffset = gMecha.PivotOffset();
            bgFrameData.terrainConfig = &gTerrainConfig;
            bgFrameData.deltaTime = 0.0f; // Freeze time
            gSceneRenderer.RenderFrame(bgFrameData);

            // Render the game over screen overlay
            Shader *uiShader = gResourceManager.Shaders().GetShader("ui");
            if (uiShader)
            {
                gGameOverScreen.Render(*uiShader, gResourceManager.GetUIQuadVAO(), gDebugText);
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        // input
        gInputController.ProcessInput(window, deltaTime);
        auto setCursorCapture = [&](bool capture)
        {
            SetCursorCapture(window, capture);
        };
        Model *mechaModel = gResourceManager.Models().GetModel("dragon_mecha");
        gDevOverlayUI.HandleInput(window, *mechaModel, gCursorCaptured, setCursorCapture);
        gDevOverlayUI.ApplyPlaybackWindowIfNeeded(*mechaModel);

        if (gDevOverlay.godzillaSpawnRequested)
        {
            if (gGodzilla)
            {
                gGodzilla->TriggerSpawn(true);
                gGodzillaSpawned = true;
            }
            gDevOverlay.godzillaSpawnRequested = false;
        }

        const MovementState &moveState = gMecha.Movement();
        const glm::vec3 mechaPosition = moveState.position;
        const float mechaYawDegrees = moveState.yawDegrees;
        const float mechaPitchDegrees = moveState.pitchDegrees;
        const float mechaRollDegrees = moveState.rollDegrees;
        const float mechaModelScale = gMecha.ModelScale();
        const glm::vec3 mechaPivotOffset = gMecha.PivotOffset();

        if (gDevOverlay.infiniteFuel)
        {
            gMecha.Flight().currentFuel = MechaPlayer::kMaxFuel;
        }

        if (gDevOverlay.godMode)
        {
            gMecha.Combat().hitPoints = MechaPlayer::kMaxHP;
        }

        // Entity-owned animation playback now handled inside each entity.

        // Update sound system listener position (camera position)
        if (gSoundManager)
        {
            const Camera &camera = gCamera.GetCamera();
            glm::vec3 cameraPos = camera.Position;
            glm::vec3 cameraForward = camera.Front;
            glm::vec3 cameraUp = camera.Up;
            gSoundManager->SetListenerPosition(cameraPos, cameraForward, cameraUp);

            // Update master volume from developer overlay
            if (gSoundManager->GetMasterVolume() != gDevOverlay.masterVolume)
            {
                gSoundManager->SetMasterVolume(gDevOverlay.masterVolume);
            }
        }

        // Prepare frame data for rendering
        glm::mat4 projection = glm::perspective(glm::radians(gCamera.GetCamera().Zoom),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                gCamera.GetNearPlane(), kSceneFarPlane);
        glm::mat4 view = gCamera.GetCamera().GetViewMatrix();

        SceneRenderer::FrameData frameData{};
        frameData.projection = projection;
        frameData.view = view;
        frameData.viewPos = gCamera.GetCamera().Position;
        frameData.mechaPosition = mechaPosition;
        frameData.mechaYawDegrees = mechaYawDegrees;
        frameData.mechaPitchDegrees = mechaPitchDegrees;
        frameData.mechaRollDegrees = mechaRollDegrees;
        frameData.mechaModelScale = mechaModelScale;
        frameData.mechaPivotOffset = mechaPivotOffset;
        frameData.terrainConfig = &gTerrainConfig;
        frameData.deltaTime = deltaTime;

        // Render complete scene
        gSceneRenderer.RenderFrame(frameData);

        // --- Combat logic ---
        // Find intended target (enemy in front of player within cone) for targeting
        // Use cone-based auto-aim: only target enemies within a forward-facing cone
        mecha::Enemy *intendedTarget = nullptr;
        float bestAlignment = -1.0f; // Best dot product (closest to forward direction)
        float targetDist = 0.0f;
        glm::vec3 targetVelocity(0.0f);

        // Get player's forward direction from camera
        glm::vec3 playerForward = gCamera.GetCamera().Front;
        playerForward = glm::normalize(playerForward);

        // Calculate cone threshold (cosine of half the cone angle)
        float coneAngleRad = glm::radians(MechaPlayer::kAutoAimConeAngleDegrees * 0.5f);
        float coneThreshold = std::cos(coneAngleRad);

        // Helper function to check if an enemy is within the cone and find the best target
        auto checkTarget = [&](mecha::Enemy *enemy, const glm::vec3 &velocity)
        {
            if (!enemy || !enemy->IsAlive())
            {
                return;
            }

            glm::vec3 toEnemy = enemy->Position() - mechaPosition;
            float dist = glm::length(toEnemy);

            // Check if within range
            if (dist >= MechaPlayer::kAutoAimRange)
            {
                return;
            }

            // Check if within cone (forward-facing)
            glm::vec3 dirToEnemy = glm::normalize(toEnemy);
            float dotProduct = glm::dot(playerForward, dirToEnemy);

            // If within cone and better aligned than current best target
            if (dotProduct >= coneThreshold && dotProduct > bestAlignment)
            {
                bestAlignment = dotProduct;
                intendedTarget = enemy;
                targetDist = dist;
                targetVelocity = velocity;
            }
        };

        // Check EnemyDrones
        for (const auto &enemy : gEnemies)
        {
            if (enemy && enemy->IsAlive())
            {
                checkTarget(enemy.get(), enemy->Velocity());
            }
        }

        // Check TurretEnemies
        for (const auto &turret : gTurrets)
        {
            if (turret && turret->IsAlive())
            {
                checkTarget(turret.get(), glm::vec3(0.0f)); // Turrets don't move
            }
        }

        // Check PortalGates
        static bool laserUnlocked = false;
        for (const auto &gate : gGates)
        {
            if (gate && gate->IsAlive())
            {
                checkTarget(gate.get(), glm::vec3(0.0f)); // Gates don't move
            }
            else if (gate && !gate->IsAlive() && !laserUnlocked)
            {
                // Portal destroyed - unlock laser
                gMecha.UnlockLaser();
                laserUnlocked = true;
                std::cout << "[Game] Laser attack unlocked!" << std::endl;
            }
        }

        if (gGodzilla)
        {
            checkTarget(gGodzilla.get(), glm::vec3(0.0f));
        }

        const bool targetAlive = intendedTarget != nullptr;
        glm::vec3 targetPos = targetAlive ? intendedTarget->Position() : glm::vec3(0.0f);
        // Apply downward bias to aim slightly lower
        if (targetAlive)
        {
            targetPos.y += MechaPlayer::kAutoAimDownBias;
        }

        // Update targeting based on target distance
        bool targetInRange = targetAlive && (targetDist < MechaPlayer::kAutoAimRange);
        gMecha.SetTargetLock(targetInRange);

        // Handle shooting input
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            gMecha.TryShoot(targetPos, targetVelocity, targetAlive, projection, view, gProjectileSystem.get());
        }

        // Handle missile launch input (E key)
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        {
            // Collect all enemies for missile targeting
            std::vector<mecha::Enemy *> allEnemies;
            allEnemies.reserve(gEnemies.size() + gTurrets.size() + gGates.size() + (gGodzilla ? 1 : 0));
            for (const auto &enemy : gEnemies)
            {
                if (enemy && enemy->IsAlive())
                {
                    allEnemies.push_back(enemy.get());
                }
            }
            for (const auto &turret : gTurrets)
            {
                if (turret && turret->IsAlive())
                {
                    allEnemies.push_back(turret.get());
                }
            }
            for (const auto &gate : gGates)
            {
                if (gate && gate->IsAlive())
                {
                    allEnemies.push_back(gate.get());
                }
            }
            if (gGodzilla && gGodzilla->IsAlive())
            {
                allEnemies.push_back(gGodzilla.get());
            }
            gMecha.TryLaunchMissiles(projection, view, gMissileSystem.get(), allEnemies);
        }

        // Handle laser attack input (Q key)
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        {
            // Collect all enemies for laser targeting
            std::vector<mecha::Enemy *> allEnemies;
            allEnemies.reserve(gEnemies.size() + gTurrets.size() + gGates.size() + (gGodzilla ? 1 : 0));
            for (const auto &enemy : gEnemies)
            {
                if (enemy && enemy->IsAlive())
                {
                    allEnemies.push_back(enemy.get());
                }
            }
            for (const auto &turret : gTurrets)
            {
                if (turret && turret->IsAlive())
                {
                    allEnemies.push_back(turret.get());
                }
            }
            for (const auto &gate : gGates)
            {
                if (gate && gate->IsAlive())
                {
                    allEnemies.push_back(gate.get());
                }
            }
            if (gGodzilla && gGodzilla->IsAlive())
            {
                allEnemies.push_back(gGodzilla.get());
            }
            gMecha.TryLaser(projection, view, allEnemies);
        }
        else
        {
            // Release laser when Q key is released
            gMecha.Laser().active = false;
        }

        // Update HUD with weapon state
        const auto &weaponState = gMecha.Weapon();
        gMecha.SetBeamState(weaponState.beamActive, weaponState.shootCooldown, MechaPlayer::kShootCooldown);

        // --- UI: Crosshair, Laser, Boost/Cooldown, Health ---
        glDisable(GL_DEPTH_TEST);

        // Calculate HUD state
        GameHUD::HUDState hudState = gGameHUD.CalculateHUDState(gMecha, SCR_WIDTH, SCR_HEIGHT, FOCUS_CIRCLE_RADIUS);

        // Convert to renderer format
        HudRenderer::HudRenderData hudData{};
        hudData.screenSize = hudState.screenSize;
        hudData.crosshairPos = hudState.crosshairPos;
        hudData.focusCircleRadius = hudState.focusCircleRadius;
        hudData.targetLocked = hudState.targetLocked;
        hudData.beamActive = hudState.beamActive;
        hudData.boostActive = hudState.boostActive;
        hudData.fuelActive = hudState.fuelActive;
        hudData.boostFill = hudState.boostFill;
        hudData.cooldownFill = hudState.cooldownFill;
        hudData.fuelFill = hudState.fuelFill;
        hudData.healthFill = hudState.healthFill;

        // Minimap data - collect all enemy positions
        hudData.playerPosition = mechaPosition;
        hudData.enemyPositions.clear();
        hudData.enemyAlive.clear();
        hudData.enemyPositions.reserve(gEnemies.size() + gTurrets.size());
        hudData.enemyAlive.reserve(gEnemies.size() + gTurrets.size());
        for (const auto &enemy : gEnemies)
        {
            if (enemy)
            {
                hudData.enemyPositions.push_back(enemy->Position());
                hudData.enemyAlive.push_back(enemy->IsAlive());
            }
        }
        for (const auto &turret : gTurrets)
        {
            if (turret)
            {
                hudData.enemyPositions.push_back(turret->Position());
                hudData.enemyAlive.push_back(turret->IsAlive());
            }
        }
        hudData.portalPositions.clear();
        hudData.portalAlive.clear();
        hudData.portalPositions.reserve(gGates.size());
        hudData.portalAlive.reserve(gGates.size());
        for (const auto &gate : gGates)
        {
            if (gate)
            {
                hudData.portalPositions.push_back(gate->Position());
                hudData.portalAlive.push_back(gate->IsAlive());
            }
        }

        hudData.godzillaVisible = static_cast<bool>(gGodzilla);
        if (gGodzilla)
        {
            hudData.godzillaPosition = gGodzilla->Position();
            hudData.godzillaAlive = gGodzilla->IsAlive();
        }

        // Boss health bar data - only show when boss is spawned and alive
        hudData.bossVisible = gGodzillaSpawned && gGodzilla && gGodzilla->IsAlive();
        if (hudData.bossVisible)
        {
            hudData.bossAlive = gGodzilla->IsAlive();
            hudData.bossHealthFill = gGodzilla->HitPoints() / gGodzilla->MaxHitPoints();
            hudData.bossName = "KAIJU";
        }

        if (!gGodzillaSpawned && !gGates.empty())
        {
            bool allGatesDestroyed = true;
            for (const auto &gate : gGates)
            {
                if (gate && gate->IsAlive())
                {
                    allGatesDestroyed = false;
                    break;
                }
            }
            if (allGatesDestroyed && gGodzilla)
            {
                gGodzilla->TriggerSpawn(false);
                gGodzillaSpawned = true;

                // Switch to boss fight music when boss spawns
                if (gBackgroundMusic)
                {
                    gBackgroundMusic->SetStage(mecha::BackgroundMusicSystem::MusicStage::BossFight, 2.0f);
                }
            }
        }

        // Track portal destruction events for objective system
        for (size_t i = 0; i < gGates.size() && i < gPreviousPortalStates.size(); ++i)
        {
            bool currentAlive = gGates[i] ? gGates[i]->IsAlive() : false;
            if (gPreviousPortalStates[i] && !currentAlive)
            {
                // Portal was alive but now dead - it was destroyed
                gObjectiveSystem.OnPortalDestroyed();

                // After second portal destroyed, upgrade missiles to launch 4 instead of 2
                if (gObjectiveSystem.GetState().portalsDestroyed >= 2 && gMissileSystem && !gMissileSystem->IsUpgraded())
                {
                    gMissileSystem->UpgradeMissiles();
                    std::cout << "[Game] Missiles upgraded! Now launching 4 missiles (2 normal + 2 mini)" << std::endl;
                }
            }
            gPreviousPortalStates[i] = currentAlive;
        }

        // Track boss defeat for objective system and music
        if (gGodzilla)
        {
            bool currentBossAlive = gGodzilla->IsAlive();
            if (gPreviousBossAlive && !currentBossAlive)
            {
                // Boss was alive but now dead - defeated
                gObjectiveSystem.OnBossDefeated();

                // Fade out music when boss dies
                if (gBackgroundMusic)
                {
                    gBackgroundMusic->FadeOut(5.0f);
                }

                // Start victory timer (5 seconds delay)
                gBossDeathTimer = 5.0f;
            }
            gPreviousBossAlive = currentBossAlive;

            // Update victory timer
            if (!currentBossAlive && gBossDeathTimer > 0.0f)
            {
                gBossDeathTimer -= deltaTime;
                if (gBossDeathTimer <= 0.0f && gGameState == GameState::Playing)
                {
                    // Transition to victory state
                    gGameState = GameState::Victory;
                    gGameOverScreen.Show(mecha::GameOverScreen::ScreenType::Victory);
                    SetCursorCapture(window, false);
                }
            }
        }

        // Check for player death
        if (gGameState == GameState::Playing && gMecha.Combat().hitPoints <= 0.0f && !gMecha.IsGodMode())
        {
            gGameState = GameState::PlayerDeath;
            gGameOverScreen.Show(mecha::GameOverScreen::ScreenType::PlayerDeath);
            SetCursorCapture(window, false);
        }

        // Set objective text for HUD
        hudData.objectiveText = gObjectiveSystem.GetObjectiveText();

        hudData.minimapWorldRange = 100.0f;         // Show 100 units around player
        hudData.playerYawDegrees = mechaYawDegrees; // Player rotation for minimap orientation

        Shader *uiShader = gResourceManager.Shaders().GetShader("ui");
        gHudRenderer.Render(hudData, *uiShader, gResourceManager.GetUIQuadVAO());

        // Render objective display below minimap
        gHudRenderer.RenderObjective(hudData, *uiShader, gResourceManager.GetUIQuadVAO(), gDebugText);

        // Render boss health bar at bottom of screen
        gHudRenderer.RenderBossHealthBar(hudData, *uiShader, gResourceManager.GetUIQuadVAO(), gDebugText);

        mecha::DeveloperOverlayUI::RenderParams overlayParams{
            *uiShader,
            gResourceManager.GetUIQuadVAO(),
            glm::vec2(static_cast<float>(SCR_WIDTH), static_cast<float>(SCR_HEIGHT)),
            *mechaModel,
            gMecha};
        gDevOverlayUI.Render(overlayParams);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);

        // Update sound system
        if (gSoundManager)
        {
            gSoundManager->Update(deltaTime);
        }

        // Update background music system
        if (gBackgroundMusic)
        {
            gBackgroundMusic->Update(deltaTime);
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Shutdown main menu
    gMainMenu.Shutdown();

    // Shutdown sound system
    if (gSoundController)
    {
        gSoundController->Shutdown();
        gSoundController.reset();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources
    glfwTerminate();
    return 0;
}
