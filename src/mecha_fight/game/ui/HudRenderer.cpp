#include "HudRenderer.h"
#include "DebugTextRenderer.h"

#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/constants.hpp>

#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    const glm::vec4 kHudBackgroundColor(0.0f, 0.0f, 0.0f, 0.35f);

    void DrawRect(Shader &shader, const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color, float fill)
    {
      shader.setVec2("rectPos", pos);
      shader.setVec2("rectSize", size);
      shader.setVec4("color", color);
      shader.setFloat("fill", fill);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void DrawFocusCircle(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      const int segments = 32;
      const glm::vec2 center = data.crosshairPos;
      const glm::vec4 &color = data.targetLocked ? data.lockedFocusColor : data.unlockedFocusColor;

      for (int i = 0; i < segments; ++i)
      {
        float angle = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
        glm::vec2 point(center.x + std::cos(angle) * data.focusCircleRadius,
                        center.y + std::sin(angle) * data.focusCircleRadius);
        DrawRect(shader, point - glm::vec2(2.0f), glm::vec2(4.0f), color, 1.0f);
      }
    }

    void DrawBeam(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      glm::vec2 center = data.crosshairPos;
      glm::vec2 direction = center - glm::vec2(data.screenSize * 0.5f);
      float len = glm::length(direction);
      if (len > 1.0f)
      {
        direction /= len;
      }
      else
      {
        direction = glm::vec2(1.0f, 0.0f);
      }

      (void)direction; // Beam currently rendered as muzzle flash at crosshair

      DrawRect(shader, center - glm::vec2(4.0f), glm::vec2(8.0f), data.beamColor, 1.0f);
    }

    void DrawBoostAndCooldownBars(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      const float margin = 20.0f;
      const glm::vec2 boostSize(300.0f, 16.0f);
      const glm::vec2 cdSize(300.0f, 8.0f);

      glm::vec2 boostPosTL(margin, data.screenSize.y - (margin + boostSize.y));
      glm::vec2 cdPosTL(margin, boostPosTL.y - 6.0f - cdSize.y);

      DrawRect(shader, boostPosTL, boostSize, kHudBackgroundColor, 1.0f);

      const glm::vec4 boostColor = data.boostActive ? data.boostActiveColor : data.boostReadyColor;
      DrawRect(shader, boostPosTL, boostSize, boostColor, glm::clamp(data.boostFill, 0.0f, 1.0f));

      DrawRect(shader, cdPosTL, cdSize, kHudBackgroundColor, 1.0f);
      DrawRect(shader, cdPosTL, cdSize, data.cooldownColor, glm::clamp(data.cooldownFill, 0.0f, 1.0f));

      const glm::vec2 fuelSize(300.0f, 12.0f);
      glm::vec2 fuelPosTL(margin, cdPosTL.y - 4.0f - fuelSize.y);

      DrawRect(shader, fuelPosTL, fuelSize, kHudBackgroundColor, 1.0f);
      const glm::vec4 fuelColor = data.fuelActive ? data.fuelActiveColor : data.fuelIdleColor;
      DrawRect(shader, fuelPosTL, fuelSize, fuelColor, glm::clamp(data.fuelFill, 0.0f, 1.0f));
    }

    void DrawHealthBar(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      const glm::vec2 hpSize(300.0f, 12.0f);
      const glm::vec2 hpPosTL(20.0f, 20.0f);

      DrawRect(shader, hpPosTL, hpSize, kHudBackgroundColor, 1.0f);
      DrawRect(shader, hpPosTL, hpSize, data.healthColor, glm::clamp(data.healthFill, 0.0f, 1.0f));
    }

    void DrawCrosshair(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      const glm::vec2 horizontalSize(22.0f, 2.0f);
      const glm::vec2 verticalSize(2.0f, 22.0f);
      const glm::vec2 posH = data.crosshairPos - glm::vec2(horizontalSize.x * 0.5f, horizontalSize.y * 0.5f);
      const glm::vec2 posV = data.crosshairPos - glm::vec2(verticalSize.x * 0.5f, verticalSize.y * 0.5f);

      DrawRect(shader, posH, horizontalSize, data.crosshairColor, 1.0f);
      DrawRect(shader, posV, verticalSize, data.crosshairColor, 1.0f);
    }

    void DrawMinimap(const HudRenderer::HudRenderData &data, Shader &shader)
    {
      const float margin = 20.0f;
      const float minimapSize = 150.0f;
      const float minimapRadius = minimapSize * 0.5f;

      // Position in top-right corner
      glm::vec2 minimapCenter(data.screenSize.x - margin - minimapRadius, margin + minimapRadius);

      // Draw minimap background (square with rounded appearance)
      const glm::vec4 minimapBgColor(0.0f, 0.0f, 0.0f, 0.6f);
      const glm::vec4 minimapBorderColor(0.3f, 0.3f, 0.3f, 0.9f);

      // Draw square background
      glm::vec2 minimapPos = minimapCenter - glm::vec2(minimapRadius);
      DrawRect(shader, minimapPos, glm::vec2(minimapSize), minimapBgColor, 1.0f);

      // Draw circular border
      const int circleSegments = 64;
      for (int i = 0; i < circleSegments; ++i)
      {
        float angle = (static_cast<float>(i) / static_cast<float>(circleSegments)) * glm::two_pi<float>();
        glm::vec2 point = minimapCenter + glm::vec2(std::cos(angle), std::sin(angle)) * minimapRadius;
        DrawRect(shader, point - glm::vec2(2.0f), glm::vec2(4.0f), minimapBorderColor, 1.0f);
      }

      // Rotate minimap based on player yaw (so the world rotates around the fixed forward indicator).
      // Player's forward direction should always point up on minimap
      float yawRad = glm::radians(data.playerYawDegrees);
      float cosYaw = std::cos(yawRad);
      float sinYaw = std::sin(yawRad);

      // Convert world offset (X, Z) to minimap screen coordinates
      // World: +Z is forward, +X is right
      // Screen: -Y is up, +X is right
      // Rotate so player's forward always points up
      auto worldToScreen = [&](const glm::vec2 &worldVec) -> glm::vec2
      {
        // worldVec.x = world X offset (right)
        // worldVec.y = world Z offset (forward)

        // Rotate by negative player yaw to counter player rotation
        float rotatedX = worldVec.x * cosYaw + worldVec.y * sinYaw;
        float rotatedZ = -worldVec.x * sinYaw + worldVec.y * cosYaw;

        // Map to screen: X stays X, Z maps to -Y (forward is up)
        return glm::vec2(rotatedX, -rotatedZ);
      };

      // Draw center point (player position) - always at center
      const glm::vec4 playerColor(0.2f, 0.8f, 1.0f, 1.0f);
      const glm::vec2 playerDotSize(6.0f);
      DrawRect(shader, minimapCenter - playerDotSize * 0.5f, playerDotSize, playerColor, 1.0f);

      // Draw player forward indicator (arrow ALWAYS pointing up - completely fixed, no rotation)
      // This is drawn BEFORE any rotation calculations, so it's never affected by player yaw
      const glm::vec4 forwardColor(0.2f, 0.8f, 1.0f, 0.8f);
      const float forwardArrowLength = 15.0f;
      // Fixed direction: always point up (positive Y in screen coords = up on screen)
      glm::vec2 forwardTip = minimapCenter + glm::vec2(0.0f, forwardArrowLength);
      glm::vec2 forwardBase = minimapCenter + glm::vec2(0.0f, forwardArrowLength * 0.5f);

      // Draw arrow shaft (vertical line pointing up)
      glm::vec2 forwardShaftSize(2.0f, forwardArrowLength * 0.5f);
      DrawRect(shader, forwardBase - glm::vec2(forwardShaftSize.x * 0.5f, forwardShaftSize.y), forwardShaftSize, forwardColor, 1.0f);

      // Draw arrow head (triangle pointing up - drawn as a small rectangle)
      glm::vec2 forwardHeadSize(6.0f, 4.0f);
      DrawRect(shader, forwardTip - glm::vec2(forwardHeadSize.x * 0.5f, forwardHeadSize.y), forwardHeadSize, forwardColor, 1.0f);

      // Draw all enemy positions
      const glm::vec4 enemyColor(1.0f, 0.2f, 0.2f, 1.0f);
      const float scale = minimapRadius / data.minimapWorldRange;
      const glm::vec2 enemyDotSize(8.0f);

      for (size_t i = 0; i < data.enemyPositions.size() && i < data.enemyAlive.size(); ++i)
      {
        if (!data.enemyAlive[i])
        {
          continue; // Skip dead enemies
        }

        // Calculate relative position (top-down view: X-Z plane)
        glm::vec3 relativePos = data.enemyPositions[i] - data.playerPosition;
        glm::vec2 worldOffset(relativePos.x, relativePos.z);
        float distance = glm::length(worldOffset);
        glm::vec2 screenOffset = worldToScreen(worldOffset);

        // Check if enemy is within minimap range
        if (distance <= data.minimapWorldRange)
        {
          // Enemy is within range - draw enemy dot
          glm::vec2 enemyScreenPos = minimapCenter + screenOffset * scale;
          DrawRect(shader, enemyScreenPos - enemyDotSize * 0.5f, enemyDotSize, enemyColor, 1.0f);
        }
        else
        {
          // Enemy is outside range - draw arrow pointing to enemy direction
          glm::vec2 direction = glm::normalize(screenOffset);
          glm::vec2 arrowBase = minimapCenter + direction * (minimapRadius - 12.0f);
          glm::vec2 arrowTip = minimapCenter + direction * (minimapRadius - 2.0f);

          // Draw arrow line
          glm::vec2 arrowDir = arrowTip - arrowBase;
          float arrowLen = glm::length(arrowDir);
          if (arrowLen > 0.0f)
          {
            glm::vec2 perp = glm::vec2(-arrowDir.y, arrowDir.x) / arrowLen;
            glm::vec2 arrowSize(arrowLen, 2.0f);
            DrawRect(shader, arrowBase - perp * 1.0f, arrowSize, enemyColor, 1.0f);

            // Draw arrow head
            glm::vec2 headSize(6.0f);
            DrawRect(shader, arrowTip - headSize * 0.5f, headSize, enemyColor, 1.0f);
          }
        }
      }

      // Draw all portal positions in purple
      const glm::vec4 portalColor(0.8f, 0.2f, 1.0f, 1.0f); // Purple color
      const glm::vec2 portalDotSize(10.0f);                // Slightly bigger than enemies

      for (size_t i = 0; i < data.portalPositions.size() && i < data.portalAlive.size(); ++i)
      {
        if (!data.portalAlive[i])
        {
          continue; // Skip destroyed portals
        }

        // Calculate relative position (top-down view: X-Z plane)
        glm::vec3 relativePos = data.portalPositions[i] - data.playerPosition;
        glm::vec2 worldOffset(relativePos.x, relativePos.z);
        float distance = glm::length(worldOffset);
        glm::vec2 screenOffset = worldToScreen(worldOffset);

        // Check if portal is within minimap range
        if (distance <= data.minimapWorldRange)
        {
          // Portal is within range - draw portal dot
          glm::vec2 portalScreenPos = minimapCenter + screenOffset * scale;
          DrawRect(shader, portalScreenPos - portalDotSize * 0.5f, portalDotSize, portalColor, 1.0f);
        }
        else
        {
          // Portal is outside range - draw arrow pointing to portal direction
          glm::vec2 direction = glm::normalize(screenOffset);
          glm::vec2 arrowBase = minimapCenter + direction * (minimapRadius - 12.0f);
          glm::vec2 arrowTip = minimapCenter + direction * (minimapRadius - 2.0f);

          // Draw arrow line
          glm::vec2 arrowDir = arrowTip - arrowBase;
          float arrowLen = glm::length(arrowDir);
          if (arrowLen > 0.0f)
          {
            glm::vec2 perp = glm::vec2(-arrowDir.y, arrowDir.x) / arrowLen;
            glm::vec2 arrowSize(arrowLen, 2.0f);
            DrawRect(shader, arrowBase - perp * 1.0f, arrowSize, portalColor, 1.0f);

            // Draw arrow head
            glm::vec2 headSize(6.0f);
            DrawRect(shader, arrowTip - headSize * 0.5f, headSize, portalColor, 1.0f);
          }
        }
      }

      // Draw Godzilla indicator
      if (data.godzillaVisible)
      {
        const glm::vec4 bossColor(0.9f, 0.3f, 1.0f, 1.0f);
        const glm::vec2 bossDotSize(14.0f);

        glm::vec3 relativePos = data.godzillaPosition - data.playerPosition;
        glm::vec2 worldOffset(relativePos.x, relativePos.z);
        float distance = glm::length(worldOffset);
        glm::vec2 screenOffset = worldToScreen(worldOffset);

        if (distance <= data.minimapWorldRange)
        {
          glm::vec2 bossScreenPos = minimapCenter + screenOffset * scale;
          DrawRect(shader, bossScreenPos - bossDotSize * 0.5f, bossDotSize, bossColor, 1.0f);
        }
        else
        {
          glm::vec2 direction = glm::normalize(screenOffset);
          glm::vec2 arrowBase = minimapCenter + direction * (minimapRadius - 18.0f);
          glm::vec2 arrowTip = minimapCenter + direction * (minimapRadius - 4.0f);

          glm::vec2 arrowDir = arrowTip - arrowBase;
          float arrowLen = glm::length(arrowDir);
          if (arrowLen > 0.0f)
          {
            glm::vec2 perp = glm::vec2(-arrowDir.y, arrowDir.x) / arrowLen;
            glm::vec2 arrowSize(arrowLen, 4.0f);
            DrawRect(shader, arrowBase - perp * 2.0f, arrowSize, bossColor, 1.0f);

            glm::vec2 headSize(8.0f);
            DrawRect(shader, arrowTip - headSize * 0.5f, headSize, bossColor, 1.0f);
          }
        }
      }

      // Draw compass indicator (North marker) - rotate based on player yaw
      const glm::vec4 compassColor(0.8f, 0.8f, 0.8f, 0.6f);
      glm::vec2 northDir = worldToScreen(glm::vec2(0.0f, 1.0f));
      if (glm::length(northDir) > 0.0f)
      {
        northDir = glm::normalize(northDir);
      }
      glm::vec2 northPos = minimapCenter + northDir * (minimapRadius - 5.0f);
      glm::vec2 compassSize(4.0f, 8.0f);
      DrawRect(shader, northPos - glm::vec2(compassSize.x * 0.5f, 0.0f), compassSize, compassColor, 1.0f);
    }
  } // namespace

  void HudRenderer::Render(const HudRenderData &data, Shader &shader, unsigned int quadVao) const
  {
    shader.use();
    shader.setVec2("screenSize", data.screenSize);
    glBindVertexArray(quadVao);

    DrawFocusCircle(data, shader);

    if (data.beamActive)
    {
      DrawBeam(data, shader);
    }

    DrawBoostAndCooldownBars(data, shader);
    DrawHealthBar(data, shader);
    DrawCrosshair(data, shader);
    DrawMinimap(data, shader);

    glBindVertexArray(0);
  }

  void HudRenderer::RenderObjective(const HudRenderData &data, Shader &shader, unsigned int quadVao, DebugTextRenderer &textRenderer) const
  {
    if (data.objectiveText.empty() || !textRenderer.IsReady())
    {
      return;
    }

    // Position objective below the minimap
    const float margin = 20.0f;
    const float minimapSize = 150.0f;
    const float objectiveBoxWidth = 200.0f;
    const float objectiveBoxHeight = 65.0f;

    // Minimap is at top-right corner
    float minimapRight = data.screenSize.x - margin;
    float minimapBottom = margin + minimapSize;

    // Position objective box below minimap, centered on minimap
    float boxX = minimapRight - objectiveBoxWidth;
    float boxY = minimapBottom + 10.0f;

    // Draw background box
    shader.use();
    shader.setVec2("screenSize", data.screenSize);
    glBindVertexArray(quadVao);

    const glm::vec4 bgColor(0.0f, 0.0f, 0.0f, 0.6f);
    const glm::vec4 borderColor(0.8f, 0.6f, 0.2f, 0.9f);

    // Background
    shader.setVec2("rectPos", glm::vec2(boxX, boxY));
    shader.setVec2("rectSize", glm::vec2(objectiveBoxWidth, objectiveBoxHeight));
    shader.setVec4("color", bgColor);
    shader.setFloat("fill", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Border - top
    shader.setVec2("rectPos", glm::vec2(boxX, boxY));
    shader.setVec2("rectSize", glm::vec2(objectiveBoxWidth, 2.0f));
    shader.setVec4("color", borderColor);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Border - bottom
    shader.setVec2("rectPos", glm::vec2(boxX, boxY + objectiveBoxHeight - 2.0f));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Border - left
    shader.setVec2("rectPos", glm::vec2(boxX, boxY));
    shader.setVec2("rectSize", glm::vec2(2.0f, objectiveBoxHeight));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Border - right
    shader.setVec2("rectPos", glm::vec2(boxX + objectiveBoxWidth - 2.0f, boxY));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    // Draw "OBJECTIVE" label
    const glm::vec3 labelColor(0.8f, 0.6f, 0.2f);
    const glm::vec3 textColor(1.0f, 1.0f, 1.0f);

    float labelX = boxX + 8.0f;
    float labelY = boxY + 18.0f;
    textRenderer.RenderText("OBJECTIVE", labelX, labelY, 0.4f, labelColor);

    // Draw objective text
    float textY = boxY + 38.0f;
    textRenderer.RenderText(data.objectiveText, labelX, textY, 0.45f, textColor);
  }

  void HudRenderer::RenderBossHealthBar(const HudRenderData &data, Shader &shader, unsigned int quadVao, DebugTextRenderer &textRenderer) const
  {
    if (!data.bossVisible || !data.bossAlive)
    {
      return;
    }

    // Boss health bar at bottom center of screen
    const float barWidth = 500.0f;
    const float barHeight = 20.0f;
    const float margin = 40.0f;

    // Center horizontally, position near bottom
    float barX = (data.screenSize.x - barWidth) * 0.5f;
    float barY = data.screenSize.y - margin - barHeight;

    shader.use();
    shader.setVec2("screenSize", data.screenSize);
    glBindVertexArray(quadVao);

    // Background (dark)
    const glm::vec4 bgColor(0.1f, 0.1f, 0.1f, 0.8f);
    shader.setVec2("rectPos", glm::vec2(barX, barY));
    shader.setVec2("rectSize", glm::vec2(barWidth, barHeight));
    shader.setVec4("color", bgColor);
    shader.setFloat("fill", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Health fill (gradient from red to orange-red)
    const glm::vec4 healthColor(0.9f, 0.2f, 0.1f, 1.0f);
    float fillWidth = barWidth * glm::clamp(data.bossHealthFill, 0.0f, 1.0f);
    if (fillWidth > 0.0f)
    {
      shader.setVec2("rectPos", glm::vec2(barX, barY));
      shader.setVec2("rectSize", glm::vec2(fillWidth, barHeight));
      shader.setVec4("color", healthColor);
      glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // Border (golden/orange)
    const glm::vec4 borderColor(0.9f, 0.6f, 0.1f, 1.0f);
    const float borderThickness = 2.0f;

    // Top border
    shader.setVec2("rectPos", glm::vec2(barX - borderThickness, barY - borderThickness));
    shader.setVec2("rectSize", glm::vec2(barWidth + borderThickness * 2.0f, borderThickness));
    shader.setVec4("color", borderColor);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Bottom border
    shader.setVec2("rectPos", glm::vec2(barX - borderThickness, barY + barHeight));
    shader.setVec2("rectSize", glm::vec2(barWidth + borderThickness * 2.0f, borderThickness));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Left border
    shader.setVec2("rectPos", glm::vec2(barX - borderThickness, barY - borderThickness));
    shader.setVec2("rectSize", glm::vec2(borderThickness, barHeight + borderThickness * 2.0f));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Right border
    shader.setVec2("rectPos", glm::vec2(barX + barWidth, barY - borderThickness));
    shader.setVec2("rectSize", glm::vec2(borderThickness, barHeight + borderThickness * 2.0f));
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    // Draw boss name label above the health bar
    if (textRenderer.IsReady())
    {
      const glm::vec3 nameColor(0.9f, 0.6f, 0.1f); // Golden color
      float nameY = barY - 25.0f; // Above the bar

      // Center the text above the bar
      float textScale = 0.6f;
      float textWidth = data.bossName.length() * 12.0f * textScale; // Approximate width
      float nameX = barX + (barWidth - textWidth) * 0.5f;

      textRenderer.RenderText(data.bossName, nameX, nameY, textScale, nameColor);
    }
  }

} // namespace mecha
