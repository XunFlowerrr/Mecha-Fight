#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

class Shader;

namespace mecha
{
  class DebugTextRenderer;

  class HudRenderer
  {
  public:
    struct HudRenderData
    {
      glm::vec2 screenSize{0.0f, 0.0f};
      glm::vec2 crosshairPos{0.0f, 0.0f};
      bool targetLocked{false};
      bool beamActive{false};
      bool boostActive{false};
      bool fuelActive{false};
      float boostFill{0.0f};
      float cooldownFill{0.0f};
      float fuelFill{0.0f};
      float healthFill{0.0f};
      float focusCircleRadius{120.0f};
      glm::vec4 lockedFocusColor{1.0f, 0.0f, 0.0f, 0.8f};
      glm::vec4 unlockedFocusColor{0.0f, 1.0f, 0.0f, 0.6f};
      glm::vec4 beamColor{1.0f, 0.2f, 0.05f, 0.95f};
      glm::vec4 boostActiveColor{1.0f, 0.6f, 0.1f, 0.9f};
      glm::vec4 boostReadyColor{0.2f, 0.85f, 0.3f, 0.9f};
      glm::vec4 cooldownColor{0.2f, 0.6f, 1.0f, 0.9f};
      glm::vec4 fuelActiveColor{0.1f, 0.9f, 1.0f, 0.9f};
      glm::vec4 fuelIdleColor{0.3f, 0.8f, 0.4f, 0.9f};
      glm::vec4 healthColor{0.9f, 0.1f, 0.2f, 0.9f};
      glm::vec4 crosshairColor{1.0f};

      // Minimap data
      glm::vec3 playerPosition{0.0f};
      std::vector<glm::vec3> enemyPositions;  // All enemy positions
      std::vector<bool> enemyAlive;           // Whether each enemy is alive
      std::vector<glm::vec3> portalPositions; // Portal gate positions
      std::vector<bool> portalAlive;          // Whether each portal is alive
      bool godzillaVisible{false};
      bool godzillaAlive{false};
      glm::vec3 godzillaPosition{0.0f};
      float minimapWorldRange{100.0f}; // World units visible in minimap
      float playerYawDegrees{0.0f};    // Player rotation for minimap orientation

      // Objective display
      std::string objectiveText;

      // Boss health bar
      bool bossVisible{false};
      bool bossAlive{false};
      float bossHealthFill{0.0f};
      std::string bossName{"BOSS"};
    };

    void Render(const HudRenderData &data, Shader &shader, unsigned int quadVao) const;
    void RenderObjective(const HudRenderData &data, Shader &shader, unsigned int quadVao, DebugTextRenderer &textRenderer) const;
    void RenderBossHealthBar(const HudRenderData &data, Shader &shader, unsigned int quadVao, DebugTextRenderer &textRenderer) const;
  };

} // namespace mecha
