#pragma once

#include "../entities/MechaPlayer.h"
#include <glm/glm.hpp>

namespace mecha
{

  /**
   * @brief Manages HUD state preparation and calculations
   *
   * Handles HUD data calculations including fill percentages for boost,
   * fuel, health, and cooldown states. Separates HUD logic from rendering.
   */
  class GameHUD
  {
  public:
    struct HUDState
    {
      // Screen properties
      glm::vec2 screenSize;
      glm::vec2 crosshairPos;
      float focusCircleRadius;

      // Combat/targeting
      bool targetLocked;
      bool beamActive;

      // Boost system
      bool boostActive;
      float boostFill;    // 0-1, how much boost remains
      float cooldownFill; // 0-1, cooldown progress

      // Flight system
      bool fuelActive;
      float fuelFill; // 0-1

      // Health
      float healthFill; // 0-1
    };

    GameHUD();

    /**
     * @brief Calculate HUD state from player and screen info
     * @param player Player entity
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     * @param focusCircleRadius Radius of targeting circle
     * @return Prepared HUD state ready for rendering
     */
    HUDState CalculateHUDState(const MechaPlayer &player,
                               unsigned int screenWidth,
                               unsigned int screenHeight,
                               float focusCircleRadius) const;

  private:
    float CalculateBoostFill(const MechaPlayer &player) const;
    float CalculateCooldownFill(const MechaPlayer &player) const;
  };

} // namespace mecha
