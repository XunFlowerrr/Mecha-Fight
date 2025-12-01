#include <glad/glad.h>

#include "DeveloperOverlayUI.h"

#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

#include "../GameplayTypes.h"
#include "../entities/MechaPlayer.h"
#include "DebugTextRenderer.h"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

#include <iomanip>
#include <sstream>
#include <vector>

namespace mecha
{

  DeveloperOverlayUI::DeveloperOverlayUI(DeveloperOverlayState &state, DebugTextRenderer &textRenderer)
      : state_(state), textRenderer_(textRenderer)
  {
  }

  void DeveloperOverlayUI::Reset(Model &model)
  {
    state_.selectedIndex = 0;
    state_.animationSpeed = 1.0f;
    state_.animationPaused = false;
    state_.playbackWindowEnabled = false;
    state_.playbackStartNormalized = 0.0f;
    state_.playbackEndNormalized = 1.0f;
    state_.playbackWindowDirty = true;
    state_.timeScale = 1.0f;
    state_.cameraDistance = 6.0f;
    state_.infiniteFuel = false;
    state_.godMode = false;
    state_.alignToTerrain = false;
    state_.noclip = false;
    state_.showMeleeHitbox = false;
    state_.godzillaSpawnRequested = false;
    model.ClearAnimationPlaybackWindow();
  }

  void DeveloperOverlayUI::ApplyPlaybackWindowIfNeeded(Model &model)
  {
    if (!state_.playbackWindowDirty)
    {
      return;
    }

    state_.playbackWindowDirty = false;

    if (!state_.playbackWindowEnabled || !model.HasAnimations())
    {
      model.ClearAnimationPlaybackWindow();
      return;
    }

    float duration = model.GetActiveAnimationDuration();
    if (duration <= 0.0f)
    {
      return;
    }

    float start = glm::clamp(duration * state_.playbackStartNormalized, 0.0f, duration);
    float end = glm::clamp(duration * state_.playbackEndNormalized, start + 0.01f, duration);
    state_.playbackStartNormalized = start / duration;
    state_.playbackEndNormalized = end / duration;
    model.SetAnimationPlaybackWindow(start, end);
  }

  void DeveloperOverlayUI::ChangeAnimationClip(Model &model, int direction)
  {
    if (!model.HasAnimations())
    {
      return;
    }

    int clipCount = model.GetAnimationClipCount();
    if (clipCount <= 0)
    {
      return;
    }

    int current = model.GetActiveAnimationIndex();
    if (current < 0)
    {
      current = 0;
    }

    current = (current + direction) % clipCount;
    if (current < 0)
    {
      current += clipCount;
    }

    model.SetActiveAnimation(current);
    state_.playbackWindowDirty = true;
  }

  void DeveloperOverlayUI::AdjustSelectedControl(Model &model, int direction)
  {
    if (direction == 0)
    {
      return;
    }

    switch (state_.selectedIndex)
    {
    case DEV_ANIMATION_CLIP:
      ChangeAnimationClip(model, direction);
      break;
    case DEV_ANIMATION_SPEED:
      state_.animationSpeed = glm::clamp(state_.animationSpeed + direction * 0.1f, 0.1f, 3.0f);
      break;
    case DEV_PLAYBACK_START:
      if (state_.playbackWindowEnabled)
      {
        state_.playbackStartNormalized = glm::clamp(state_.playbackStartNormalized + direction * 0.02f, 0.0f, 0.98f);
        if (state_.playbackStartNormalized > state_.playbackEndNormalized - 0.02f)
        {
          state_.playbackStartNormalized = state_.playbackEndNormalized - 0.02f;
        }
        state_.playbackWindowDirty = true;
      }
      break;
    case DEV_PLAYBACK_END:
      if (state_.playbackWindowEnabled)
      {
        state_.playbackEndNormalized = glm::clamp(state_.playbackEndNormalized + direction * 0.02f, 0.02f, 1.0f);
        if (state_.playbackEndNormalized < state_.playbackStartNormalized + 0.02f)
        {
          state_.playbackEndNormalized = state_.playbackStartNormalized + 0.02f;
        }
        state_.playbackWindowDirty = true;
      }
      break;
    case DEV_TIME_SCALE:
      state_.timeScale = glm::clamp(state_.timeScale + direction * 0.1f, 0.1f, 2.0f);
      break;
    case DEV_CAMERA_DISTANCE:
      state_.cameraDistance = glm::clamp(state_.cameraDistance + direction * 0.25f, 3.0f, 12.0f);
      break;
    case DEV_MASTER_VOLUME:
      state_.masterVolume = glm::clamp(state_.masterVolume + direction * 0.1f, 0.0f, kDevOverlayMaxMasterVolume);
      break;
    case DEV_ANIMATION_PAUSE:
    case DEV_PLAYBACK_ENABLE:
    case DEV_INFINITE_FUEL:
    case DEV_GOD_MODE:
    case DEV_ALIGN_TERRAIN:
    case DEV_NOCLIP:
    case DEV_MELEE_HITBOX:
    case DEV_SPAWN_GODZILLA:
    case DEV_RESET_DEFAULTS:
      ActivateSelectedControl(model);
      break;
    default:
      break;
    }
  }

  void DeveloperOverlayUI::ActivateSelectedControl(Model &model)
  {
    switch (state_.selectedIndex)
    {
    case DEV_ANIMATION_PAUSE:
      state_.animationPaused = !state_.animationPaused;
      break;
    case DEV_PLAYBACK_ENABLE:
      state_.playbackWindowEnabled = !state_.playbackWindowEnabled;
      state_.playbackWindowDirty = true;
      break;
    case DEV_INFINITE_FUEL:
      state_.infiniteFuel = !state_.infiniteFuel;
      break;
    case DEV_GOD_MODE:
      state_.godMode = !state_.godMode;
      break;
    case DEV_ALIGN_TERRAIN:
      state_.alignToTerrain = !state_.alignToTerrain;
      break;
    case DEV_NOCLIP:
      state_.noclip = !state_.noclip;
      break;
    case DEV_MELEE_HITBOX:
      state_.showMeleeHitbox = !state_.showMeleeHitbox;
      break;
    case DEV_SPAWN_GODZILLA:
      state_.godzillaSpawnRequested = true;
      break;
    case DEV_RESET_DEFAULTS:
      Reset(model);
      break;
    case DEV_ANIMATION_CLIP:
      ChangeAnimationClip(model, 1);
      break;
    default:
      break;
    }
  }

  bool DeveloperOverlayUI::IsKeyPressedOnce(GLFWwindow *window, int key)
  {
    int state = glfwGetKey(window, key);
    bool pressed = (state == GLFW_PRESS);
    bool triggered = pressed && !keyLatch_[key];
    keyLatch_[key] = pressed;
    return triggered;
  }

  void DeveloperOverlayUI::HandleInput(GLFWwindow *window, Model &model, bool cursorCaptured, const std::function<void(bool)> &setCursorCapture)
  {
    if (IsKeyPressedOnce(window, GLFW_KEY_F3))
    {
      state_.visible = !state_.visible;
      if (state_.visible)
      {
        cursorCaptureBeforeOverlay_ = cursorCaptured;
        setCursorCapture(false);
      }
      else
      {
        setCursorCapture(cursorCaptureBeforeOverlay_);
      }
    }

    if (IsKeyPressedOnce(window, GLFW_KEY_F2) && !state_.visible)
    {
      cursorCaptureBeforeOverlay_ = !cursorCaptured;
      setCursorCapture(cursorCaptureBeforeOverlay_);
    }

    if (!state_.visible)
    {
      return;
    }

    if (IsKeyPressedOnce(window, GLFW_KEY_UP))
    {
      state_.selectedIndex = (state_.selectedIndex - 1 + DEV_CONTROL_COUNT) % DEV_CONTROL_COUNT;
    }
    if (IsKeyPressedOnce(window, GLFW_KEY_DOWN))
    {
      state_.selectedIndex = (state_.selectedIndex + 1) % DEV_CONTROL_COUNT;
    }
    if (IsKeyPressedOnce(window, GLFW_KEY_LEFT))
    {
      AdjustSelectedControl(model, -1);
    }
    if (IsKeyPressedOnce(window, GLFW_KEY_RIGHT))
    {
      AdjustSelectedControl(model, 1);
    }

    if (IsKeyPressedOnce(window, GLFW_KEY_ENTER) || IsKeyPressedOnce(window, GLFW_KEY_SPACE))
    {
      ActivateSelectedControl(model);
    }
  }

  void DeveloperOverlayUI::Render(const RenderParams &params) const
  {
    if (!state_.visible || !textRenderer_.IsReady())
    {
      return;
    }

    Shader &uiShader = params.uiShader;
    uiShader.use();
    glBindVertexArray(params.quadVao);

    auto drawText = [&](const std::string &text, float x, float y, float scale, const glm::vec3 &color)
    {
      textRenderer_.RenderText(text, x, y, scale, color);
      glBindVertexArray(params.quadVao);
    };

    const float panelWidth = 400.0f;
    const float panelPadding = 16.0f;
    const float rowHeight = 26.0f;
    const glm::vec2 panelPos(params.screenSize.x - panelWidth - 24.0f, 70.0f);
    const float headerHeight = 60.0f;
    const float panelHeight = headerHeight + DEV_CONTROL_COUNT * rowHeight + 90.0f;

    uiShader.setVec2("rectPos", panelPos);
    uiShader.setVec2("rectSize", glm::vec2(panelWidth, panelHeight));
    uiShader.setVec4("color", glm::vec4(0.02f, 0.02f, 0.02f, 0.78f));
    uiShader.setFloat("fill", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    const bool hasAnimations = params.model.HasAnimations();
    const int clipCount = params.model.GetAnimationClipCount();
    const int currentClip = params.model.GetActiveAnimationIndex();
    const float duration = params.model.GetActiveAnimationDuration();

    struct Row
    {
      std::string label;
      std::string value;
      bool disabled;
    };

    std::vector<Row> rows;
    rows.reserve(DEV_CONTROL_COUNT);

    {
      std::ostringstream oss;
      if (clipCount > 0)
      {
        oss << (currentClip + 1) << " / " << clipCount;
      }
      else
      {
        oss << "N/A";
      }
      rows.push_back({"Animation Clip", oss.str(), !hasAnimations});
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << state_.animationSpeed << "x";
      rows.push_back({"Animation Speed", oss.str(), false});
    }

    rows.push_back({"Animation State", state_.animationPaused ? "Paused" : "Playing", false});
    rows.push_back({"Playback Window", state_.playbackWindowEnabled ? "Enabled" : "Disabled", !hasAnimations});

    auto formatPlaybackValue = [&](bool isStart)
    {
      std::ostringstream oss;
      if (!hasAnimations || duration <= 0.0f)
      {
        oss << "N/A";
      }
      else
      {
        float norm = isStart ? state_.playbackStartNormalized : state_.playbackEndNormalized;
        float seconds = norm * duration;
        oss << std::fixed << std::setprecision(2) << seconds << "s (" << std::setprecision(0) << norm * 100.0f << "%)";
      }
      return oss.str();
    };

    rows.push_back({"Playback Start", formatPlaybackValue(true), !hasAnimations || !state_.playbackWindowEnabled});
    rows.push_back({"Playback End", formatPlaybackValue(false), !hasAnimations || !state_.playbackWindowEnabled});

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << state_.timeScale << "x";
      rows.push_back({"Time Scale", oss.str(), false});
    }

    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(1) << state_.cameraDistance << "u";
      rows.push_back({"Camera Distance", oss.str(), false});
    }

    rows.push_back({"Infinite Fuel", state_.infiniteFuel ? "On" : "Off", false});
    rows.push_back({"God Mode", state_.godMode ? "On" : "Off", false});
    rows.push_back({"Align To Terrain", state_.alignToTerrain ? "On" : "Off", false});
    rows.push_back({"Noclip Mode", state_.noclip ? "Enabled" : "Disabled", false});
    rows.push_back({"Melee Hitbox Debug", state_.showMeleeHitbox ? "On" : "Off", false});
    rows.push_back({"Spawn Godzilla", "Trigger", false});
    {
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(0) << (state_.masterVolume * 100.0f) << "%";
      rows.push_back({"Master Volume", oss.str(), false});
    }
    rows.push_back({"Reset (Enter)", "Defaults", false});

    const float textScale = 0.52f;
    const float headerTextScale = 0.6f;
    glm::vec3 titleColor(0.95f, 0.95f, 1.0f);
    glm::vec3 valueColor(0.5f, 0.9f, 1.0f);
    glm::vec3 disabledColor(0.5f, 0.5f, 0.5f);
    glm::vec3 highlightColor(0.2f, 0.7f, 1.0f);
    glm::vec3 highlightTextColor(1.0f, 1.0f, 1.0f);

    float textX = panelPos.x + panelPadding;
    float y = panelPos.y + 26.0f;
    drawText("Developer Panel (F3)", textX, y, headerTextScale, titleColor);
    y += 18.0f;
    drawText("Arrows navigate  |  Enter toggles  |  F2 toggles cursor", textX, y, 0.45f, glm::vec3(0.7f));
    y += 20.0f;

    for (size_t i = 0; i < rows.size(); ++i)
    {
      float rowTop = y + static_cast<float>(i) * rowHeight - 12.0f;
      const bool isSelected = static_cast<int>(i) == state_.selectedIndex;
      float rowY = y + static_cast<float>(i) * rowHeight;
      if (isSelected)
      {
        uiShader.setVec2("rectPos", glm::vec2(panelPos.x + 4.0f, rowTop + 4.0f));
        uiShader.setVec2("rectSize", glm::vec2(panelWidth - 8.0f, rowHeight));
        uiShader.setVec4("color", glm::vec4(highlightColor, 0.25f));
        uiShader.setFloat("fill", 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glm::vec3 caretColor = rows[i].disabled ? disabledColor : highlightColor;
        drawText("▶", panelPos.x + 8.0f, rowY, textScale, caretColor);
      }

      float labelX = panelPos.x + panelPadding + (isSelected ? 18.0f : 0.0f);
      glm::vec3 activeLabelColor = rows[i].disabled ? disabledColor : (isSelected ? highlightTextColor : titleColor);
      glm::vec3 activeValueColor = rows[i].disabled ? disabledColor : (isSelected ? highlightColor : valueColor);

      drawText(rows[i].label, labelX, rowY, textScale, activeLabelColor);
      float valueX = panelPos.x + panelWidth - 150.0f;
      drawText(rows[i].value, valueX, rowY, textScale, activeValueColor);
    }

    float statsY = panelPos.y + panelHeight - 52.0f;
    drawText("Stats", textX, statsY, headerTextScale, titleColor);
    statsY += 18.0f;

    const MovementState &movementState = params.player.Movement();
    std::ostringstream posStream;
    posStream << std::fixed << std::setprecision(1) << movementState.position.x << ", " << movementState.position.y << ", " << movementState.position.z;
    drawText("Mecha Pos: " + posStream.str(), textX, statsY, 0.48f, valueColor);
    statsY += 16.0f;

    std::ostringstream speedStream;
    speedStream << std::fixed << std::setprecision(2) << movementState.forwardSpeed;
    drawText("Speed: " + speedStream.str() + " u/s", textX, statsY, 0.48f, valueColor);
    statsY += 16.0f;

    const FlightState &flightState = params.player.Flight();
    std::ostringstream fuelStream;
    fuelStream << std::fixed << std::setprecision(0) << (flightState.currentFuel / MechaPlayer::kMaxFuel * 100.0f);
    drawText("Fuel: " + fuelStream.str() + "%", textX, statsY, 0.48f, valueColor);
    statsY += 16.0f;

    const CombatState &combatState = params.player.Combat();
    std::ostringstream hpStream;
    hpStream << std::fixed << std::setprecision(0) << combatState.hitPoints;
    drawText("HP: " + hpStream.str(), textX, statsY, 0.48f, valueColor);
  }

} // namespace mecha
