#include "MainMenu.h"
#include <learnopengl/shader_m.h>
#include <learnopengl/filesystem.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION_DISABLED
#include <stb_image.h>

namespace mecha
{

    MainMenu::MainMenu()
    {
    }

    MainMenu::~MainMenu()
    {
        Shutdown();
    }

    bool MainMenu::Initialize(unsigned int screenWidth, unsigned int screenHeight, const std::string &backgroundPath)
    {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        m_state = MenuState::Active;
        m_lastFrameTime = static_cast<float>(glfwGetTime());

        // Initialize text rendering with FreeType
        std::string fontPath = FileSystem::getPath("resources/fonts/Antonio-Bold.ttf");
        if (!InitTextRendering(fontPath, 48))
        {
            std::cerr << "[MainMenu] Warning: Failed to initialize text rendering" << std::endl;
        }

        // Load background texture
        m_backgroundTexture = LoadTexture(backgroundPath);
        m_hasBackground = (m_backgroundTexture != 0);

        if (!m_hasBackground)
        {
            std::cerr << "[MainMenu] Warning: Failed to load background image: " << backgroundPath << std::endl;
        }
        else
        {
            std::cout << "[MainMenu] Background loaded successfully" << std::endl;
        }

        // Create menu items
        CreateMenuItems();

        std::cout << "[MainMenu] Initialized (" << screenWidth << "x" << screenHeight << ")" << std::endl;
        return true;
    }

    bool MainMenu::InitTextRendering(const std::string &fontPath, unsigned int fontSize)
    {
        // Load text shader (reuse dev_text shaders)
        const std::string vertexPath = FileSystem::getPath("src/mecha_fight/shaders/dev_text.vs");
        const std::string fragmentPath = FileSystem::getPath("src/mecha_fight/shaders/dev_text.fs");
        m_textShader = std::make_unique<Shader>(vertexPath.c_str(), fragmentPath.c_str());

        // Setup projection matrix
        m_textProjection = glm::ortho(0.0f, static_cast<float>(m_screenWidth),
                                      static_cast<float>(m_screenHeight), 0.0f);
        m_textShader->use();
        m_textShader->setMat4("projection", m_textProjection);
        m_textShader->setInt("text", 0);

        // Setup VAO/VBO for text rendering
        glGenVertexArrays(1, &m_textVAO);
        glGenBuffers(1, &m_textVBO);
        glBindVertexArray(m_textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Initialize FreeType
        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            std::cerr << "[MainMenu] ERROR: Could not init FreeType Library" << std::endl;
            return false;
        }

        FT_Face face;
        if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
        {
            std::cerr << "[MainMenu] ERROR: Failed to load font: " << fontPath << std::endl;
            FT_Done_FreeType(ft);
            return false;
        }

        FT_Set_Pixel_Sizes(face, 0, fontSize);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Load ASCII characters
        for (unsigned char c = 0; c < 128; ++c)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cerr << "[MainMenu] Warning: Failed to load glyph for char " << static_cast<int>(c) << std::endl;
                continue;
            }

            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                         face->glyph->bitmap.width,
                         face->glyph->bitmap.rows,
                         0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            Glyph glyph;
            glyph.textureID = texture;
            glyph.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            glyph.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);
            m_glyphs.insert(std::make_pair(static_cast<char>(c), glyph));
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        m_textInitialized = true;
        std::cout << "[MainMenu] Text rendering initialized with font: " << fontPath << std::endl;
        return true;
    }

    void MainMenu::RenderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color)
    {
        if (!m_textInitialized || !m_textShader)
        {
            return;
        }

        m_textShader->use();
        m_textShader->setVec3("textColor", color);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(m_textVAO);

        for (char c : text)
        {
            auto it = m_glyphs.find(c);
            if (it == m_glyphs.end())
            {
                continue;
            }

            const Glyph &glyph = it->second;
            float xpos = x + glyph.bearing.x * scale;
            float ypos = y - (glyph.size.y - glyph.bearing.y) * scale;
            float w = glyph.size.x * scale;
            float h = glyph.size.y * scale;

            float vertices[6][4] = {
                {xpos, ypos + h, 0.0f, 1.0f},
                {xpos + w, ypos, 1.0f, 0.0f},
                {xpos, ypos, 0.0f, 0.0f},

                {xpos, ypos + h, 0.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 1.0f},
                {xpos + w, ypos, 1.0f, 0.0f}};

            glBindTexture(GL_TEXTURE_2D, glyph.textureID);
            glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            x += (glyph.advance >> 6) * scale;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    float MainMenu::GetTextWidth(const std::string &text, float scale)
    {
        float width = 0.0f;
        for (char c : text)
        {
            auto it = m_glyphs.find(c);
            if (it != m_glyphs.end())
            {
                width += (it->second.advance >> 6) * scale;
            }
        }
        return width;
    }

    void MainMenu::Shutdown()
    {
        if (m_backgroundTexture != 0)
        {
            glDeleteTextures(1, &m_backgroundTexture);
            m_backgroundTexture = 0;
        }
        m_hasBackground = false;

        // Cleanup text rendering resources
        for (auto &pair : m_glyphs)
        {
            glDeleteTextures(1, &pair.second.textureID);
        }
        m_glyphs.clear();

        if (m_textVAO != 0)
        {
            glDeleteVertexArrays(1, &m_textVAO);
            m_textVAO = 0;
        }
        if (m_textVBO != 0)
        {
            glDeleteBuffers(1, &m_textVBO);
            m_textVBO = 0;
        }
        m_textShader.reset();
        m_textInitialized = false;

        m_menuItems.clear();
    }

    void MainMenu::CreateMenuItems()
    {
        m_menuItems.clear();

        float centerX = m_screenWidth * 0.5f;
        float centerY = m_screenHeight * 0.5f;
        float buttonWidth = 300.0f;
        float buttonHeight = 60.0f;
        float buttonSpacing = 80.0f;

        // Start Game button
        MenuItem startButton;
        startButton.text = "Start Game";
        startButton.position = glm::vec2(centerX, centerY + 20.0f);
        startButton.size = glm::vec2(buttonWidth, buttonHeight);
        startButton.onSelect = [this]()
        {
            m_state = MenuState::StartGame;
        };
        m_menuItems.push_back(startButton);

        // Fullscreen/Windowed toggle button
        MenuItem displayButton;
        displayButton.text = m_isFullscreen ? "Windowed" : "Fullscreen";
        displayButton.position = glm::vec2(centerX, centerY + 20.0f + buttonSpacing);
        displayButton.size = glm::vec2(buttonWidth, buttonHeight);
        displayButton.onSelect = [this]()
        {
            ToggleFullscreen();
        };
        m_menuItems.push_back(displayButton);

        // Quit button
        MenuItem quitButton;
        quitButton.text = "Quit";
        quitButton.position = glm::vec2(centerX, centerY + 20.0f + buttonSpacing * 2);
        quitButton.size = glm::vec2(buttonWidth, buttonHeight);
        quitButton.onSelect = [this]()
        {
            m_state = MenuState::Quit;
        };
        m_menuItems.push_back(quitButton);

        m_selectedIndex = 0;
        if (!m_menuItems.empty())
        {
            m_menuItems[m_selectedIndex].hovered = true;
        }
    }

    void MainMenu::ToggleFullscreen()
    {
        if (!m_window)
        {
            std::cerr << "[MainMenu] Cannot toggle fullscreen: window not set" << std::endl;
            return;
        }

        m_isFullscreen = !m_isFullscreen;

        if (m_isFullscreen)
        {
            glfwGetWindowPos(m_window, &m_windowedPosX, &m_windowedPosY);
            glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

            GLFWmonitor *monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);

            glfwSetWindowPos(m_window, 0, 0);
            glfwSetWindowSize(m_window, mode->width, mode->height);

            std::cout << "[MainMenu] Switched to fullscreen-like mode (" << mode->width << "x" << mode->height << ")" << std::endl;
        }
        else
        {
            glfwSetWindowPos(m_window, m_windowedPosX, m_windowedPosY);
            glfwSetWindowSize(m_window, m_windowedWidth, m_windowedHeight);

            std::cout << "[MainMenu] Switched to windowed (" << m_windowedWidth << "x" << m_windowedHeight << ")" << std::endl;
        }

        // Update button text
        for (auto &item : m_menuItems)
        {
            if (item.text == "Fullscreen" || item.text == "Windowed")
            {
                item.text = m_isFullscreen ? "Windowed" : "Fullscreen";
            }
        }
    }

    void MainMenu::ProcessInput(GLFWwindow *window)
    {
        if (m_state != MenuState::Active)
        {
            return;
        }

        m_window = window;

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
        m_animationTime += deltaTime;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        UpdateHoverState(mouseX, mouseY);

        // Mouse click
        bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mouseDown && !m_mousePressed)
        {
            for (size_t i = 0; i < m_menuItems.size(); ++i)
            {
                if (m_menuItems[i].hovered && m_menuItems[i].onSelect)
                {
                    m_menuItems[i].onSelect();
                    break;
                }
            }
        }
        m_mousePressed = mouseDown;

        // Keyboard navigation - Up
        bool upDown = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        if (upDown && !m_upPressed && !m_menuItems.empty())
        {
            m_menuItems[m_selectedIndex].hovered = false;
            m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_menuItems.size())) % static_cast<int>(m_menuItems.size());
            m_menuItems[m_selectedIndex].hovered = true;
        }
        m_upPressed = upDown;

        // Keyboard navigation - Down
        bool downDown = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
        if (downDown && !m_downPressed && !m_menuItems.empty())
        {
            m_menuItems[m_selectedIndex].hovered = false;
            m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_menuItems.size());
            m_menuItems[m_selectedIndex].hovered = true;
        }
        m_downPressed = downDown;

        // Keyboard selection - Enter/Space
        bool enterDown = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (enterDown && !m_enterPressed)
        {
            if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_menuItems.size()))
            {
                auto &item = m_menuItems[m_selectedIndex];
                if (item.onSelect)
                {
                    item.onSelect();
                }
            }
        }
        m_enterPressed = enterDown;
    }

    void MainMenu::UpdateHoverState(double mouseX, double mouseY)
    {
        glm::vec2 mousePos(static_cast<float>(mouseX), static_cast<float>(mouseY));

        bool anyHovered = false;
        for (size_t i = 0; i < m_menuItems.size(); ++i)
        {
            auto &item = m_menuItems[i];
            bool wasHovered = item.hovered;
            item.hovered = IsPointInRect(mousePos, item.position, item.size);

            if (item.hovered && !wasHovered)
            {
                m_selectedIndex = static_cast<int>(i);
                anyHovered = true;
            }
            else if (item.hovered)
            {
                anyHovered = true;
            }
        }

        if (!anyHovered && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_menuItems.size()))
        {
            m_menuItems[m_selectedIndex].hovered = true;
        }
    }

    bool MainMenu::IsPointInRect(const glm::vec2 &point, const glm::vec2 &rectCenter, const glm::vec2 &rectSize)
    {
        glm::vec2 halfSize = rectSize * 0.5f;
        return (point.x >= rectCenter.x - halfSize.x && point.x <= rectCenter.x + halfSize.x &&
                point.y >= rectCenter.y - halfSize.y && point.y <= rectCenter.y + halfSize.y);
    }

    unsigned int MainMenu::LoadTexture(const std::string &path)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

        if (data)
        {
            GLenum format = GL_RGB;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            std::cout << "[MainMenu] Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
        }
        else
        {
            std::cerr << "[MainMenu] Failed to load texture: " << path << std::endl;
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }

        return textureID;
    }

    void MainMenu::Render(Shader &shader, unsigned int quadVAO)
    {
        if (m_state != MenuState::Active)
        {
            return;
        }

        shader.use();
        shader.setVec2("screenSize", glm::vec2(m_screenWidth, m_screenHeight));
        glBindVertexArray(quadVAO);

        // Render background
        if (m_hasBackground)
        {
            shader.setVec2("rectPos", glm::vec2(0.0f, 0.0f));
            shader.setVec2("rectSize", glm::vec2(m_screenWidth, m_screenHeight));
            shader.setVec4("color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            shader.setFloat("fill", 1.0f);
            shader.setInt("useTexture", 1);
            shader.setInt("uTexture", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            shader.setInt("useTexture", 0);
        }
        else
        {
            shader.setVec2("rectPos", glm::vec2(0.0f, 0.0f));
            shader.setVec2("rectSize", glm::vec2(m_screenWidth, m_screenHeight));
            shader.setVec4("color", glm::vec4(0.05f, 0.08f, 0.15f, 1.0f));
            shader.setFloat("fill", 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Dark overlay
        shader.setVec2("rectPos", glm::vec2(0.0f, 0.0f));
        shader.setVec2("rectSize", glm::vec2(m_screenWidth, m_screenHeight));
        shader.setVec4("color", m_overlayColor);
        shader.setFloat("fill", 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);

        // Render title text
        if (m_textInitialized)
        {
            std::string titleText = "MECHA FIGHT";
            float titleScale = 1.5f;
            float titleWidth = GetTextWidth(titleText, titleScale);
            float titleX = (m_screenWidth - titleWidth) * 0.5f;
            float titleY = m_screenHeight * 0.22f;

            RenderText(titleText, titleX, titleY, titleScale, m_titleColor);

            // Subtitle
            std::string subtitleText = "Combat Arena";
            float subtitleScale = 0.7f;
            float subtitleWidth = GetTextWidth(subtitleText, subtitleScale);
            float subtitleX = (m_screenWidth - subtitleWidth) * 0.5f;
            float subtitleY = titleY + 50.0f;

            RenderText(subtitleText, subtitleX, subtitleY, subtitleScale, glm::vec3(0.7f, 0.7f, 0.8f));
        }

        // Render menu buttons
        shader.use();
        shader.setVec2("screenSize", glm::vec2(m_screenWidth, m_screenHeight));
        glBindVertexArray(quadVAO);

        for (size_t i = 0; i < m_menuItems.size(); ++i)
        {
            const auto &item = m_menuItems[i];
            glm::vec2 topLeft = item.position - item.size * 0.5f;

            // Button background
            float pulseAlpha = item.hovered ? (0.1f * std::sin(m_animationTime * 5.0f) + 0.1f) : 0.0f;
            glm::vec4 bgColor = item.hovered ? m_buttonHoverColor : m_buttonColor;
            bgColor.a += pulseAlpha;

            shader.setVec2("rectPos", topLeft);
            shader.setVec2("rectSize", item.size);
            shader.setVec4("color", bgColor);
            shader.setFloat("fill", 1.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Button border
            glm::vec4 borderColor = item.hovered ? glm::vec4(0.2f, 0.8f, 1.0f, 1.0f) : glm::vec4(0.4f, 0.5f, 0.6f, 0.8f);
            float bw = 2.0f;

            // Top
            shader.setVec2("rectPos", topLeft);
            shader.setVec2("rectSize", glm::vec2(item.size.x, bw));
            shader.setVec4("color", borderColor);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Bottom
            shader.setVec2("rectPos", glm::vec2(topLeft.x, topLeft.y + item.size.y - bw));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Left
            shader.setVec2("rectPos", topLeft);
            shader.setVec2("rectSize", glm::vec2(bw, item.size.y));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Right
            shader.setVec2("rectPos", glm::vec2(topLeft.x + item.size.x - bw, topLeft.y));
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Selection indicator arrow
            if (item.hovered)
            {
                float arrowOffset = std::sin(m_animationTime * 4.0f) * 5.0f;
                float arrowX = topLeft.x - 25.0f + arrowOffset;
                float arrowY = item.position.y - 8.0f;

                shader.setVec4("color", glm::vec4(0.2f, 0.8f, 1.0f, 1.0f));
                shader.setVec2("rectPos", glm::vec2(arrowX, arrowY));
                shader.setVec2("rectSize", glm::vec2(12.0f, 4.0f));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                shader.setVec2("rectPos", glm::vec2(arrowX, arrowY + 12.0f));
                glDrawArrays(GL_TRIANGLES, 0, 6);
                shader.setVec2("rectPos", glm::vec2(arrowX + 8.0f, arrowY + 4.0f));
                shader.setVec2("rectSize", glm::vec2(4.0f, 8.0f));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        glBindVertexArray(0);

        // Render button text with FreeType
        if (m_textInitialized)
        {
            for (size_t i = 0; i < m_menuItems.size(); ++i)
            {
                const auto &item = m_menuItems[i];
                float textScale = 0.5f;
                float textWidth = GetTextWidth(item.text, textScale);
                float textX = item.position.x - textWidth * 0.5f;
                // With top-left origin projection, y increases downward
                // RenderText places baseline at y, glyphs render upward (y - offset)
                // To center: baseline should be at center + some offset to account for text going up
                // Font size 48, scale 0.5 = effective 24px. Baseline to top is ~70% of that
                float textY = item.position.y; // Baseline slightly below center

                glm::vec3 textColor = item.hovered ? m_buttonHoverTextColor : m_buttonTextColor;
                RenderText(item.text, textX, textY, textScale, textColor);
            }

            // Render footer hint
            std::string hintText = "Use Arrow Keys or Mouse  |  Enter to Select";
            float hintScale = 0.4f;
            float hintWidth = GetTextWidth(hintText, hintScale);
            float hintX = (m_screenWidth - hintWidth) * 0.5f;
            float hintY = m_screenHeight - 40.0f;

            RenderText(hintText, hintX, hintY, hintScale, glm::vec3(0.5f, 0.5f, 0.5f));
        }
    }

    void MainMenu::Resize(unsigned int width, unsigned int height)
    {
        m_screenWidth = width;
        m_screenHeight = height;

        // Update text projection matrix
        if (m_textInitialized && m_textShader)
        {
            m_textProjection = glm::ortho(0.0f, static_cast<float>(width),
                                          static_cast<float>(height), 0.0f);
            m_textShader->use();
            m_textShader->setMat4("projection", m_textProjection);
        }

        CreateMenuItems();
    }

    void MainMenu::Reset()
    {
        m_state = MenuState::Active;
        m_selectedIndex = 0;

        for (auto &item : m_menuItems)
        {
            item.hovered = false;
        }

        if (!m_menuItems.empty())
        {
            m_menuItems[0].hovered = true;
        }
    }

} // namespace mecha
