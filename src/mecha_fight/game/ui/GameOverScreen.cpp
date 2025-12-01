#include "GameOverScreen.h"
#include "DebugTextRenderer.h"
#include <learnopengl/shader_m.h>
#include <iostream>
#include <cmath>

namespace mecha
{

  GameOverScreen::GameOverScreen()
  {
  }

  bool GameOverScreen::Initialize(unsigned int screenWidth, unsigned int screenHeight)
  {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    return true;
  }

  void GameOverScreen::Show(ScreenType type)
  {
    if (type == ScreenType::None)
    {
      Hide();
      return;
    }

    m_screenType = type;
    m_currentResult = SelectionResult::None;
    m_selectedIndex = 0;
    m_fadeAlpha = 0.0f;
    m_animationTime = 0.0f;

    if (type == ScreenType::PlayerDeath)
    {
      CreateDeathMenuItems();
      std::cout << "[GameOverScreen] Showing death screen" << std::endl;
    }
    else if (type == ScreenType::Victory)
    {
      CreateVictoryMenuItems();
      m_victoryPending = false;
      std::cout << "[GameOverScreen] Showing victory screen" << std::endl;
    }
  }

  void GameOverScreen::Hide()
  {
    m_screenType = ScreenType::None;
    m_menuItems.clear();
    m_currentResult = SelectionResult::None;
    m_victoryPending = false;
    m_victoryTimer = 0.0f;
  }

  void GameOverScreen::StartVictorySequence()
  {
    m_victoryPending = true;
    m_victoryTimer = 0.0f;
    std::cout << "[GameOverScreen] Victory sequence started, waiting " << kVictoryDelay << " seconds" << std::endl;
  }

  void GameOverScreen::ProcessInput(GLFWwindow *window)
  {
    if (m_screenType == ScreenType::None || m_victoryPending)
    {
      return;
    }

    // Get mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    UpdateHoverState(mouseX, mouseY);

    // Mouse click
    bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseDown && !m_mousePressed)
    {
      for (size_t i = 0; i < m_menuItems.size(); ++i)
      {
        if (m_menuItems[i].hovered)
        {
          m_currentResult = m_menuItems[i].result;
          m_menuItems[i].selected = true;
          std::cout << "[GameOverScreen] Selected: " << m_menuItems[i].text << std::endl;
          break;
        }
      }
    }
    m_mousePressed = mouseDown;

    // Keyboard navigation
    bool upDown = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool downDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool enterDown = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (upDown && !m_upPressed)
    {
      m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_menuItems.size())) % static_cast<int>(m_menuItems.size());
      // Update hover state for keyboard selection
      for (size_t i = 0; i < m_menuItems.size(); ++i)
      {
        m_menuItems[i].hovered = (static_cast<int>(i) == m_selectedIndex);
      }
    }
    m_upPressed = upDown;

    if (downDown && !m_downPressed)
    {
      m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_menuItems.size());
      for (size_t i = 0; i < m_menuItems.size(); ++i)
      {
        m_menuItems[i].hovered = (static_cast<int>(i) == m_selectedIndex);
      }
    }
    m_downPressed = downDown;

    if (enterDown && !m_enterPressed)
    {
      if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_menuItems.size()))
      {
        m_currentResult = m_menuItems[m_selectedIndex].result;
        m_menuItems[m_selectedIndex].selected = true;
        std::cout << "[GameOverScreen] Selected via keyboard: " << m_menuItems[m_selectedIndex].text << std::endl;
      }
    }
    m_enterPressed = enterDown;
  }

  void GameOverScreen::Update(float deltaTime)
  {
    // Handle victory delay
    if (m_victoryPending)
    {
      m_victoryTimer += deltaTime;
      if (m_victoryTimer >= kVictoryDelay)
      {
        Show(ScreenType::Victory);
      }
      return;
    }

    if (m_screenType == ScreenType::None)
    {
      return;
    }

    // Fade in animation
    if (m_fadeAlpha < 1.0f)
    {
      m_fadeAlpha = std::min(1.0f, m_fadeAlpha + kFadeSpeed * deltaTime);
    }

    m_animationTime += deltaTime;
  }

  void GameOverScreen::Render(Shader &shader, unsigned int quadVAO, DebugTextRenderer &textRenderer)
  {
    if (m_screenType == ScreenType::None && !m_victoryPending)
    {
      return;
    }

    // During victory pending, don't render the screen yet
    if (m_victoryPending)
    {
      return;
    }

    if (!textRenderer.IsReady())
    {
      return;
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader.use();
    shader.setVec2("screenSize", glm::vec2(m_screenWidth, m_screenHeight));
    glBindVertexArray(quadVAO);

    // Draw semi-transparent overlay
    glm::vec4 overlayWithFade = m_overlayColor;
    overlayWithFade.a *= m_fadeAlpha;
    shader.setVec2("rectPos", glm::vec2(0.0f, 0.0f));
    shader.setVec2("rectSize", glm::vec2(m_screenWidth, m_screenHeight));
    shader.setVec4("color", overlayWithFade);
    shader.setFloat("fill", 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render title
    float centerX = m_screenWidth * 0.5f;
    float titleY = m_screenHeight * 0.3f;

    std::string titleText;
    glm::vec3 titleColor;

    if (m_screenType == ScreenType::PlayerDeath)
    {
      titleText = "YOU DIED";
      titleColor = m_deathTitleColor;
    }
    else
    {
      titleText = "VICTORY!";
      titleColor = m_victoryTitleColor;
    }

    // Pulsing effect for title
    float pulse = 1.0f + 0.1f * std::sin(m_animationTime * 3.0f);
    float titleScale = 2.0f * pulse * m_fadeAlpha;

    // Estimate text width (approximate: ~20 pixels per character at scale 1.0)
    float titleWidth = titleText.length() * 20.0f * titleScale;
    textRenderer.RenderText(titleText, centerX - titleWidth * 0.5f, titleY, titleScale, titleColor * m_fadeAlpha);

    // Render subtitle for victory
    if (m_screenType == ScreenType::Victory)
    {
      std::string subtitle = "You have defeated the boss!";
      float subtitleScale = 0.8f;
      float subtitleWidth = subtitle.length() * 20.0f * subtitleScale;
      textRenderer.RenderText(subtitle, centerX - subtitleWidth * 0.5f, titleY + 60.0f, subtitleScale,
                              glm::vec3(0.9f, 0.9f, 0.9f) * m_fadeAlpha);
    }

    // Render menu buttons
    float buttonY = m_screenHeight * 0.5f;
    float buttonSpacing = 70.0f;

    for (size_t i = 0; i < m_menuItems.size(); ++i)
    {
      MenuItem &item = m_menuItems[i];
      float itemY = buttonY + i * buttonSpacing;
      item.position = glm::vec2(centerX, itemY);

      // Draw button background
      glm::vec4 btnColor = item.hovered ? m_buttonHoverColor : m_buttonColor;
      btnColor.a *= m_fadeAlpha;

      shader.use();
      shader.setVec2("screenSize", glm::vec2(m_screenWidth, m_screenHeight));
      glBindVertexArray(quadVAO);

      shader.setVec2("rectPos", glm::vec2(item.position.x - item.size.x * 0.5f, item.position.y - item.size.y * 0.5f));
      shader.setVec2("rectSize", item.size);
      shader.setVec4("color", btnColor);
      shader.setFloat("fill", 1.0f);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      // Draw border
      glm::vec4 borderColor = item.hovered ? glm::vec4(0.4f, 0.7f, 1.0f, 0.9f) : glm::vec4(0.3f, 0.4f, 0.5f, 0.7f);
      borderColor.a *= m_fadeAlpha;

      // Top border
      shader.setVec2("rectPos", glm::vec2(item.position.x - item.size.x * 0.5f, item.position.y - item.size.y * 0.5f));
      shader.setVec2("rectSize", glm::vec2(item.size.x, 2.0f));
      shader.setVec4("color", borderColor);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      // Bottom border
      shader.setVec2("rectPos", glm::vec2(item.position.x - item.size.x * 0.5f, item.position.y + item.size.y * 0.5f - 2.0f));
      glDrawArrays(GL_TRIANGLES, 0, 6);

      // Draw button text
      glm::vec3 textColor = item.hovered ? m_buttonHoverTextColor : m_buttonTextColor;
      float textScale = 0.9f;
      float textWidth = item.text.length() * 20.0f * textScale;
      float textX = item.position.x - textWidth * 0.5f;
      float textY = item.position.y - 10.0f;

      textRenderer.RenderText(item.text, textX, textY, textScale, textColor * m_fadeAlpha);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
  }

  void GameOverScreen::Resize(unsigned int width, unsigned int height)
  {
    m_screenWidth = width;
    m_screenHeight = height;
  }

  GameOverScreen::SelectionResult GameOverScreen::GetAndClearResult()
  {
    SelectionResult result = m_currentResult;
    m_currentResult = SelectionResult::None;
    return result;
  }

  void GameOverScreen::CreateDeathMenuItems()
  {
    m_menuItems.clear();

    float buttonWidth = 280.0f;
    float buttonHeight = 50.0f;
    glm::vec2 size(buttonWidth, buttonHeight);

    MenuItem continueItem;
    continueItem.text = "Continue";
    continueItem.size = size;
    continueItem.result = SelectionResult::Continue;
    m_menuItems.push_back(continueItem);

    MenuItem godModeItem;
    godModeItem.text = "God Mode";
    godModeItem.size = size;
    godModeItem.result = SelectionResult::GodMode;
    m_menuItems.push_back(godModeItem);

    MenuItem menuItem;
    menuItem.text = "Return to Menu";
    menuItem.size = size;
    menuItem.result = SelectionResult::ReturnToMenu;
    m_menuItems.push_back(menuItem);

    // Set first item as selected
    if (!m_menuItems.empty())
    {
      m_menuItems[0].hovered = true;
    }
  }

  void GameOverScreen::CreateVictoryMenuItems()
  {
    m_menuItems.clear();

    float buttonWidth = 280.0f;
    float buttonHeight = 50.0f;
    glm::vec2 size(buttonWidth, buttonHeight);

    MenuItem menuItem;
    menuItem.text = "Return to Menu";
    menuItem.size = size;
    menuItem.result = SelectionResult::ReturnToMenu;
    m_menuItems.push_back(menuItem);

    if (!m_menuItems.empty())
    {
      m_menuItems[0].hovered = true;
    }
  }

  void GameOverScreen::UpdateHoverState(double mouseX, double mouseY)
  {
    glm::vec2 mousePos(static_cast<float>(mouseX), static_cast<float>(mouseY));

    bool anyHovered = false;
    for (size_t i = 0; i < m_menuItems.size(); ++i)
    {
      m_menuItems[i].hovered = IsPointInRect(mousePos, m_menuItems[i].position, m_menuItems[i].size);
      if (m_menuItems[i].hovered)
      {
        m_selectedIndex = static_cast<int>(i);
        anyHovered = true;
      }
    }

    // If mouse is not over any button, keep keyboard selection highlighted
    if (!anyHovered && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_menuItems.size()))
    {
      m_menuItems[m_selectedIndex].hovered = true;
    }
  }

  bool GameOverScreen::IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectCenter, const glm::vec2 &rectSize)
  {
    glm::vec2 halfSize = rectSize * 0.5f;
    return point.x >= rectCenter.x - halfSize.x &&
           point.x <= rectCenter.x + halfSize.x &&
           point.y >= rectCenter.y - halfSize.y &&
           point.y <= rectCenter.y + halfSize.y;
  }

} // namespace mecha
