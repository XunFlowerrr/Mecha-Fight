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

  /**
   * @brief Main menu screen displayed before the game starts
   *
   * Features:
   * - Background image
   * - Title with FreeType text rendering
   * - Menu buttons (Start Game, Fullscreen/Windowed, Quit)
   * - Simple hover/selection effects
   */
  class MainMenu
  {
  public:
    enum class MenuState
    {
      Active,    // Menu is being displayed
      StartGame, // Player selected "Start Game"
      Quit       // Player selected "Quit"
    };

    struct MenuItem
    {
      std::string text;
      glm::vec2 position; // Center position in pixels
      glm::vec2 size;     // Button size in pixels
      bool hovered = false;
      bool selected = false;
      std::function<void()> onSelect;
    };

    MainMenu();
    ~MainMenu();

    /**
     * @brief Initialize menu resources
     * @param screenWidth Screen width in pixels
     * @param screenHeight Screen height in pixels
     * @param backgroundPath Path to background image
     * @return true if initialization successful
     */
    bool Initialize(unsigned int screenWidth, unsigned int screenHeight, const std::string &backgroundPath);

    /**
     * @brief Cleanup menu resources
     */
    void Shutdown();

    /**
     * @brief Process input for the menu
     * @param window GLFW window
     */
    void ProcessInput(GLFWwindow *window);

    /**
     * @brief Render the menu
     * @param shader UI shader to use
     * @param quadVAO VAO for quad rendering
     */
    void Render(Shader &shader, unsigned int quadVAO);

    /**
     * @brief Update screen dimensions
     */
    void Resize(unsigned int width, unsigned int height);

    /**
     * @brief Get current menu state
     */
    MenuState GetState() const { return m_state; }

    /**
     * @brief Reset menu to active state
     */
    void Reset();

    /**
     * @brief Check if menu is active
     */
    bool IsActive() const { return m_state == MenuState::Active; }

    /**
     * @brief Check if fullscreen mode is enabled
     */
    bool IsFullscreen() const { return m_isFullscreen; }

    /**
     * @brief Set the GLFW window reference for fullscreen toggle
     */
    void SetWindow(GLFWwindow *window) { m_window = window; }

  private:
    // Text rendering glyph structure
    struct Glyph
    {
      unsigned int textureID = 0;
      glm::ivec2 size = glm::ivec2(0);
      glm::ivec2 bearing = glm::ivec2(0);
      unsigned int advance = 0;
    };

    void CreateMenuItems();
    void UpdateHoverState(double mouseX, double mouseY);
    bool IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectCenter, const glm::vec2 &rectSize);
    unsigned int LoadTexture(const std::string &path);
    void ToggleFullscreen();

    // Text rendering methods
    bool InitTextRendering(const std::string &fontPath, unsigned int fontSize);
    void RenderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color);
    float GetTextWidth(const std::string &text, float scale);

    MenuState m_state = MenuState::Active;
    unsigned int m_screenWidth = 1600;
    unsigned int m_screenHeight = 900;

    // Window reference for fullscreen toggle
    GLFWwindow *m_window = nullptr;
    bool m_isFullscreen = true;
    int m_windowedWidth = 1280;
    int m_windowedHeight = 720;
    int m_windowedPosX = 100;
    int m_windowedPosY = 100;

    // Background texture
    unsigned int m_backgroundTexture = 0;
    bool m_hasBackground = false;

    // Menu items
    std::vector<MenuItem> m_menuItems;
    int m_selectedIndex = 0;

    // Input state for debouncing
    bool m_mousePressed = false;
    bool m_enterPressed = false;
    bool m_upPressed = false;
    bool m_downPressed = false;

    // Animation
    float m_animationTime = 0.0f;
    float m_lastFrameTime = 0.0f;

    // Text rendering resources
    std::map<char, Glyph> m_glyphs;
    unsigned int m_textVAO = 0;
    unsigned int m_textVBO = 0;
    std::unique_ptr<Shader> m_textShader;
    glm::mat4 m_textProjection = glm::mat4(1.0f);
    bool m_textInitialized = false;

    // Colors
    glm::vec3 m_titleColor = glm::vec3(1.0f, 0.9f, 0.2f);
    glm::vec3 m_buttonTextColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 m_buttonHoverTextColor = glm::vec3(0.2f, 0.8f, 1.0f);
    glm::vec4 m_buttonColor = glm::vec4(0.1f, 0.15f, 0.25f, 0.85f);
    glm::vec4 m_buttonHoverColor = glm::vec4(0.15f, 0.25f, 0.4f, 0.95f);
    glm::vec4 m_overlayColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
  };

} // namespace mecha
