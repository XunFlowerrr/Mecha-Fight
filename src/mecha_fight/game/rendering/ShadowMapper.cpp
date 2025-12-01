#include "ShadowMapper.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace mecha
{

  ShadowMapper::ShadowMapper()
      : m_depthMapFBO(0),
        m_depthMap(0),
        m_lightSpaceMatrix(1.0f),
        m_initialized(false)
  {
  }

  ShadowMapper::~ShadowMapper()
  {
    Cleanup();
  }

  bool ShadowMapper::Init(const Config &config)
  {
    if (m_initialized)
    {
      Cleanup();
    }

    m_config = config;

    // Create framebuffer for depth map
    glGenFramebuffers(1, &m_depthMapFBO);

    // Create depth texture
    glGenTextures(1, &m_depthMap);
    glBindTexture(GL_TEXTURE_2D, m_depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 m_config.width, m_config.height,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cout << "[ShadowMapper] ERROR: Shadow framebuffer is not complete!" << std::endl;
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      Cleanup();
      return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    UpdateLightSpaceMatrix();

    m_initialized = true;
    std::cout << "[ShadowMapper] Initialized with " << m_config.width << "x" << m_config.height << " depth map" << std::endl;
    return true;
  }

  void ShadowMapper::BeginShadowPass()
  {
    if (!m_initialized)
    {
      return;
    }

    glViewport(0, 0, m_config.width, m_config.height);
    glBindFramebuffer(GL_FRAMEBUFFER, m_depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  void ShadowMapper::EndShadowPass()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void ShadowMapper::SetLightPosition(const glm::vec3 &position)
  {
    m_config.lightPosition = position;
    UpdateLightSpaceMatrix();
  }

  void ShadowMapper::UpdateLightSpaceMatrix()
  {
    glm::mat4 lightProjection = glm::ortho(
        m_config.orthoLeft, m_config.orthoRight,
        m_config.orthoBottom, m_config.orthoTop,
        m_config.nearPlane, m_config.farPlane);

    glm::mat4 lightView = glm::lookAt(
        m_config.lightPosition,
        m_config.target,
        glm::vec3(0.0f, 1.0f, 0.0f));

    m_lightSpaceMatrix = lightProjection * lightView;
  }

  void ShadowMapper::Cleanup()
  {
    if (m_depthMapFBO != 0)
    {
      glDeleteFramebuffers(1, &m_depthMapFBO);
      m_depthMapFBO = 0;
    }

    if (m_depthMap != 0)
    {
      glDeleteTextures(1, &m_depthMap);
      m_depthMap = 0;
    }

    m_initialized = false;
  }

} // namespace mecha
