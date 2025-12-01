#pragma once

#include <glm/glm.hpp>
#include <learnopengl/shader_m.h>

namespace mecha
{

  /**
   * @brief Handles shadow mapping setup and rendering
   *
   * Encapsulates shadow map FBO, depth texture, and light space transformations.
   * Provides methods for shadow pass rendering and accessing the depth map.
   */
  class ShadowMapper
  {
  public:
    struct Config
    {
      unsigned int width = 2048;
      unsigned int height = 2048;
      glm::vec3 lightPosition = glm::vec3(10.0f, 10.0f, 10.0f);
      glm::vec3 target = glm::vec3(0.0f);
      float orthoLeft = -25.0f;
      float orthoRight = 25.0f;
      float orthoBottom = -25.0f;
      float orthoTop = 25.0f;
      float nearPlane = 1.0f;
      float farPlane = 50.0f;
    };

    ShadowMapper();
    ~ShadowMapper();

    // Non-copyable
    ShadowMapper(const ShadowMapper &) = delete;
    ShadowMapper &operator=(const ShadowMapper &) = delete;

    /**
     * @brief Initialize shadow mapping resources
     * @param config Shadow map configuration
     * @return true if initialization succeeded
     */
    bool Init(const Config &config);

    /**
     * @brief Begin shadow pass rendering
     *
     * Binds shadow FBO and sets up viewport. Call this before rendering
     * scene geometry from light's perspective.
     */
    void BeginShadowPass();

    /**
     * @brief End shadow pass rendering
     *
     * Unbinds shadow FBO and restores default framebuffer.
     */
    void EndShadowPass();

    /**
     * @brief Get the light space transformation matrix
     * @return Combined projection * view matrix from light's perspective
     */
    glm::mat4 GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }

    /**
     * @brief Get the shadow map depth texture ID
     * @return OpenGL texture ID for the depth map
     */
    unsigned int GetDepthMapTexture() const { return m_depthMap; }

    /**
     * @brief Get shadow map dimensions
     */
    unsigned int GetWidth() const { return m_config.width; }
    unsigned int GetHeight() const { return m_config.height; }

    /**
     * @brief Get light position
     */
    glm::vec3 GetLightPosition() const { return m_config.lightPosition; }

    /**
     * @brief Update light position (recalculates light space matrix)
     */
    void SetLightPosition(const glm::vec3 &position);

  private:
    void UpdateLightSpaceMatrix();
    void Cleanup();

    Config m_config;
    unsigned int m_depthMapFBO;
    unsigned int m_depthMap;
    glm::mat4 m_lightSpaceMatrix;
    bool m_initialized;
  };

} // namespace mecha
