#pragma once

#include <glm/glm.hpp>
#include <learnopengl/shader_m.h>
#include <learnopengl/model.h>
#include "ShadowMapper.h"
#include "SSAORenderer.h"
#include "ResourceManager.h"
#include "../entities/MechaPlayer.h"
#include "../entities/EnemyDrone.h"
#include "../systems/ProjectileSystem.h"
#include "../placeholder/TerrainPlaceholder.h"
#include "../../core/GameWorld.h"

namespace mecha
{

  /**
   * @brief Handles all scene rendering including shadow passes and main rendering
   *
   * Encapsulates the rendering pipeline:
   * 1. Shadow depth map generation
   * 2. Main scene rendering with shadows
   * 3. Entity rendering via GameWorld
   */
  class SceneRenderer
  {
  public:
    struct RenderConfig
    {
      unsigned int screenWidth;
      unsigned int screenHeight;
      float nearPlane;
      float farPlane;
      glm::vec3 clearColor;
      glm::vec3 lightIntensity{1.0f, 1.0f, 1.0f};
      bool showLightDebug{false};
      float lightMarkerScale{3.0f};
      bool enableSSAO{true};
      float ssaoRadius{0.8f};
      float ssaoBias{0.05f};
      float ssaoPower{1.2f};
      float ssaoStrength{0.85f};
      bool enableSkybox{true};
      float skyboxIntensity{1.0f};
      glm::vec3 skyboxTint{1.0f, 1.0f, 1.0f};
    };

    struct FrameData
    {
      // Camera
      glm::mat4 projection;
      glm::mat4 view;
      glm::vec3 viewPos;

      // Mecha state
      glm::vec3 mechaPosition;
      float mechaYawDegrees;
      float mechaPitchDegrees;
      float mechaRollDegrees;
      float mechaModelScale;
      glm::vec3 mechaPivotOffset;

      // Terrain
      const TerrainConfig *terrainConfig;

      // Time
      float deltaTime;
    };

    SceneRenderer();
    ~SceneRenderer();

    /**
     * @brief Initialize renderer with configuration
     */
    bool Initialize(const RenderConfig &config);

    /**
     * @brief Set dependencies for rendering
     */
    void SetDependencies(ResourceManager *resourceMgr, ShadowMapper *shadowMapper, GameWorld *world);

    /**
     * @brief Render complete frame (shadow pass + main scene)
     */
    void RenderFrame(const FrameData &frameData);

  private:
    RenderConfig m_config;
    ResourceManager *m_resourceMgr = nullptr;
    ShadowMapper *m_shadowMapper = nullptr;
    GameWorld *m_world = nullptr;
    SSAORenderer m_ssaoRenderer;
    bool m_ssaoInitialized{false};
    unsigned int m_skyboxVAO{0};
    unsigned int m_skyboxVBO{0};

    void RenderShadowPass(const FrameData &frameData);
    void RenderSSAOGeometry(const FrameData &frameData);
    void EvaluateSSAO(const FrameData &frameData);
    void RenderMainScene(const FrameData &frameData);
    void RenderSkybox(const FrameData &frameData);
    void RenderTerrain(const FrameData &frameData, const glm::mat4 &lightSpaceMatrix);
    void RenderEntities(const FrameData &frameData);
    void RenderLightDebug(const FrameData &frameData);
    void RenderFullscreenQuad();
    bool ShouldUseSSAO() const;
    void CreateSkyboxGeometry();
    void DestroySkyboxGeometry();
  };

} // namespace mecha
