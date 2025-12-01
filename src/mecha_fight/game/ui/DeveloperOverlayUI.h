#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <array>
#include <functional>
#include <string>
#include <vector>

struct GLFWwindow;
class Model;
class Shader;

namespace mecha
{

  class MechaPlayer;
  class DebugTextRenderer;

  inline constexpr float kDevOverlayMaxMasterVolume = 2.0f;
  inline constexpr float kDevOverlayDefaultMasterVolume = 1.3f;

  struct DeveloperOverlayState
  {
    bool visible = false;
    int selectedIndex = 0;
    float animationSpeed = 1.0f;
    bool animationPaused = false;
    bool playbackWindowEnabled = false;
    float playbackStartNormalized = 0.0f;
    float playbackEndNormalized = 1.0f;
    bool playbackWindowDirty = false;
    float timeScale = 1.0f;
    float cameraDistance = 6.0f;
    bool infiniteFuel = false;
    bool godMode = false;
    bool alignToTerrain = false;
    bool noclip = false;
    bool showMeleeHitbox = false;
    bool godzillaSpawnRequested = false;
    float masterVolume = kDevOverlayDefaultMasterVolume; // Master sound volume (0.0 to kDevOverlayMaxMasterVolume)
  };

  class DeveloperOverlayUI
  {
  public:
    struct RenderParams
    {
      Shader &uiShader;
      unsigned int quadVao{0};
      glm::vec2 screenSize{0.0f};
      const Model &model;
      const MechaPlayer &player;
    };

    DeveloperOverlayUI(DeveloperOverlayState &state, DebugTextRenderer &textRenderer);

    void Reset(Model &model);
    void ApplyPlaybackWindowIfNeeded(Model &model);
    void HandleInput(GLFWwindow *window, Model &model, bool cursorCaptured, const std::function<void(bool)> &setCursorCapture);
    void Render(const RenderParams &params) const;

    DeveloperOverlayState &State() { return state_; }
    const DeveloperOverlayState &State() const { return state_; }

  private:
    enum DevControl
    {
      DEV_ANIMATION_CLIP = 0,
      DEV_ANIMATION_SPEED,
      DEV_ANIMATION_PAUSE,
      DEV_PLAYBACK_ENABLE,
      DEV_PLAYBACK_START,
      DEV_PLAYBACK_END,
      DEV_TIME_SCALE,
      DEV_CAMERA_DISTANCE,
      DEV_INFINITE_FUEL,
      DEV_GOD_MODE,
      DEV_ALIGN_TERRAIN,
      DEV_NOCLIP,
      DEV_MELEE_HITBOX,
    DEV_SPAWN_GODZILLA,
    DEV_MASTER_VOLUME,
      DEV_RESET_DEFAULTS,
      DEV_CONTROL_COUNT
    };

    bool IsKeyPressedOnce(GLFWwindow *window, int key);
    void ChangeAnimationClip(Model &model, int direction);
    void AdjustSelectedControl(Model &model, int direction);
    void ActivateSelectedControl(Model &model);

    DeveloperOverlayState &state_;
    DebugTextRenderer &textRenderer_;
    std::array<bool, GLFW_KEY_LAST + 1> keyLatch_{};
    bool cursorCaptureBeforeOverlay_ = true;
  };

} // namespace mecha
