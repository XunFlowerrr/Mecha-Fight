#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <memory>

class Shader;

namespace mecha
{

  class DebugTextRenderer;

  /**
   * @brief Game over / victory screen
   *
   * Handles two states:
   * 1. Player Death - Shows options: Continue, God Mode, Return to Menu
   * 2. Victory - Shows victory message and Return to Menu option
   */
  class GameOverScreen
  {
  public:
    enum class ScreenType
    {
      None,
      PlayerDeath,
      Victory
    };

    enum class SelectionResult
    {
      None,
      Continue,    // Respawn with full health
      GodMode,     // Continue with god mode enabled
      ReturnToMenu // Go back to main menu
    };

    struct MenuItem
    {
      std::string text;
      glm::vec2 position;
      glm::vec2 size;
      bool hovered = false;
      bool selected = false;
      SelectionResult result = SelectionResult::None;
    };

    GameOverScreen();
    ~GameOverScreen() = default;

    /**
     * @brief Initialize screen resources
     */
    bool Initialize(unsigned int screenWidth, unsigned int screenHeight);

    /**
     * @brief Show the game over screen
     * @param type Type of screen to show
     */
    void Show(ScreenType type);

    /**
     * @brief Hide the screen
     */
    void Hide();

    /**
     * @brief Process input
     * @param window GLFW window
     */
    void ProcessInput(GLFWwindow *window);

    /**
     * @brief Update the screen (for animations, victory delay, etc.)
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Render the screen
     * @param shader UI shader
     * @param quadVAO Quad VAO for rendering
     * @param textRenderer Text renderer for FreeType text
     */
    void Render(Shader &shader, unsigned int quadVAO, DebugTextRenderer &textRenderer);

    /**
     * @brief Update screen dimensions
     */
    void Resize(unsigned int width, unsigned int height);

    /**
     * @brief Check if screen is active
     */
    bool IsActive() const { return m_screenType != ScreenType::None; }

    /**
     * @brief Get the current selection result (resets after reading)
     */
    SelectionResult GetAndClearResult();

    /**
     * @brief Get current screen type
     */
    ScreenType GetScreenType() const { return m_screenType; }

    /**
     * @brief Start victory sequence (called when boss dies, waits then shows victory)
     */
    void StartVictorySequence();

    /**
     * @brief Check if victory sequence is pending
     */
    bool IsVictoryPending() const { return m_victoryPending; }

  private:
    void CreateDeathMenuItems();
    void CreateVictoryMenuItems();
    void UpdateHoverState(double mouseX, double mouseY);
    bool IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectCenter, const glm::vec2 &rectSize);

    ScreenType m_screenType = ScreenType::None;
    SelectionResult m_currentResult = SelectionResult::None;

    unsigned int m_screenWidth = 1600;
    unsigned int m_screenHeight = 900;

    std::vector<MenuItem> m_menuItems;
    int m_selectedIndex = 0;

    // Input debouncing
    bool m_mousePressed = false;
    bool m_enterPressed = false;
    bool m_upPressed = false;
    bool m_downPressed = false;

    // Victory sequence
    bool m_victoryPending = false;
    float m_victoryTimer = 0.0f;
    static constexpr float kVictoryDelay = 5.0f;

    // Animation
    float m_animationTime = 0.0f;
    float m_fadeAlpha = 0.0f;
    static constexpr float kFadeSpeed = 2.0f;

    // Colors
    glm::vec3 m_deathTitleColor = glm::vec3(1.0f, 0.2f, 0.2f);
    glm::vec3 m_victoryTitleColor = glm::vec3(1.0f, 0.85f, 0.2f);
    glm::vec3 m_buttonTextColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 m_buttonHoverTextColor = glm::vec3(0.2f, 0.8f, 1.0f);
    glm::vec4 m_buttonColor = glm::vec4(0.1f, 0.15f, 0.25f, 0.85f);
    glm::vec4 m_buttonHoverColor = glm::vec4(0.15f, 0.25f, 0.4f, 0.95f);
    glm::vec4 m_overlayColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.7f);
  };

} // namespace mecha
