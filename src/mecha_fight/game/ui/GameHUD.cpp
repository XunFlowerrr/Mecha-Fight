#include "GameHUD.h"
#include <algorithm>

namespace mecha
{

  GameHUD::GameHUD()
  {
  }

  GameHUD::HUDState GameHUD::CalculateHUDState(const MechaPlayer &player,
                                               unsigned int screenWidth,
                                               unsigned int screenHeight,
                                               float focusCircleRadius) const
  {
    HUDState state{};

    // Screen properties
    state.screenSize = glm::vec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    state.crosshairPos = state.screenSize * 0.5f;
    state.focusCircleRadius = focusCircleRadius;

    // Get player HUD state
    const auto &hudState = player.GetHudState();

    // Combat/targeting
    state.targetLocked = hudState.targetLocked;
    state.beamActive = hudState.beamActive;

    // Boost system
    state.boostActive = hudState.boostActive;
    state.boostFill = CalculateBoostFill(player);
    state.cooldownFill = CalculateCooldownFill(player);

    // Flight system
    state.fuelActive = hudState.flying;
    state.fuelFill = glm::clamp(hudState.maxFuel > 0.0f ? hudState.fuel / hudState.maxFuel : 0.0f, 0.0f, 1.0f);

    // Health
    state.healthFill = glm::clamp(hudState.maxHealth > 0.0f ? hudState.health / hudState.maxHealth : 0.0f, 0.0f, 1.0f);

    return state;
  }

  float GameHUD::CalculateBoostFill(const MechaPlayer &player) const
  {
    const auto &hudState = player.GetHudState();

    if (hudState.boostActive)
    {
      // During boost, show remaining time
      const float boostDuration = hudState.boostDuration > 0.0f ? hudState.boostDuration : 1.0f;
      return glm::clamp(hudState.boostTimeLeft / boostDuration, 0.0f, 1.0f);
    }
    else if (hudState.boostCooldownLeft <= 0.0f)
    {
      // Boost ready
      return 1.0f;
    }

    // Boost not ready yet
    return 0.0f;
  }

  float GameHUD::CalculateCooldownFill(const MechaPlayer &player) const
  {
    const auto &hudState = player.GetHudState();

    if (hudState.boostCooldownLeft > 0.0f)
    {
      // Show cooldown progress
      const float boostCooldown = hudState.boostCooldown > 0.0f ? hudState.boostCooldown : 1.0f;
      return 1.0f - glm::clamp(hudState.boostCooldownLeft / boostCooldown, 0.0f, 1.0f);
    }

    // No cooldown, fully ready
    return 1.0f;
  }

} // namespace mecha
