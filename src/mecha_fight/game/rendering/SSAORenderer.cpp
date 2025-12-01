#include "SSAORenderer.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <vector>

namespace mecha
{

  namespace
  {
    float RandomFloat(float minValue, float maxValue)
    {
      static std::mt19937 rng(std::random_device{}());
      std::uniform_real_distribution<float> dist(minValue, maxValue);
      return dist(rng);
    }
  } // namespace

  SSAORenderer::SSAORenderer()
  {
  }

  SSAORenderer::~SSAORenderer()
  {
    Cleanup();
  }

  bool SSAORenderer::Init(const Config &config)
  {
    Cleanup();
    m_config = config;

    if (m_config.width == 0 || m_config.height == 0 || m_config.kernelSize == 0)
    {
      return false;
    }

    GenerateKernel();
    CreateNoiseTexture();
    CreateGBuffer();
    CreateSSAOBuffers();
    CreateQuad();

    m_initialized = true;
    return true;
  }

  void SSAORenderer::Resize(unsigned int width, unsigned int height)
  {
    if (!m_initialized)
    {
      return;
    }
    m_config.width = width;
    m_config.height = height;
    CreateGBuffer();
    CreateSSAOBuffers();
  }

  void SSAORenderer::Cleanup()
  {
    DestroyBuffers();
    m_kernel.clear();
    m_initialized = false;
  }

  void SSAORenderer::BeginGeometryPass()
  {
    if (!m_initialized)
    {
      return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void SSAORenderer::EndGeometryPass()
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void SSAORenderer::GenerateKernel()
  {
    m_kernel.clear();
    m_kernel.reserve(m_config.kernelSize);

    for (unsigned int i = 0; i < m_config.kernelSize; ++i)
    {
      glm::vec3 sample(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), RandomFloat(0.0f, 1.0f));
      sample = glm::normalize(sample);
      sample *= RandomFloat(0.0f, 1.0f);

      float scale = static_cast<float>(i) / static_cast<float>(m_config.kernelSize);
      scale = glm::mix(0.1f, 1.0f, scale * scale);
      sample *= scale;
      m_kernel.push_back(sample);
    }
  }

  void SSAORenderer::CreateNoiseTexture()
  {
    std::vector<glm::vec3> noiseData;
    noiseData.reserve(16);
    for (int i = 0; i < 16; ++i)
    {
      glm::vec3 noise(RandomFloat(-1.0f, 1.0f), RandomFloat(-1.0f, 1.0f), 0.0f);
      noiseData.push_back(noise);
    }

    if (m_noiseTexture == 0)
    {
      glGenTextures(1, &m_noiseTexture);
    }
    glBindTexture(GL_TEXTURE_2D, m_noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, noiseData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  void SSAORenderer::CreateGBuffer()
  {
    if (m_gBufferFBO == 0)
    {
      glGenFramebuffers(1, &m_gBufferFBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);

    if (m_gPosition == 0)
    {
      glGenTextures(1, &m_gPosition);
    }
    glBindTexture(GL_TEXTURE_2D, m_gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_config.width, m_config.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

    if (m_gNormal == 0)
    {
      glGenTextures(1, &m_gNormal);
    }
    glBindTexture(GL_TEXTURE_2D, m_gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_config.width, m_config.height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

    if (m_depthRBO == 0)
    {
      glGenRenderbuffers(1, &m_depthRBO);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_config.width, m_config.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRBO);

    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cerr << "[SSAORenderer] G-buffer incomplete" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void SSAORenderer::CreateSSAOBuffers()
  {
    if (m_ssaoFBO == 0)
    {
      glGenFramebuffers(1, &m_ssaoFBO);
    }
    if (m_ssaoColorBuffer == 0)
    {
      glGenTextures(1, &m_ssaoColorBuffer);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
    glBindTexture(GL_TEXTURE_2D, m_ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_config.width, m_config.height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorBuffer, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cerr << "[SSAORenderer] SSAO pass framebuffer incomplete" << std::endl;
    }

    if (m_ssaoBlurFBO == 0)
    {
      glGenFramebuffers(1, &m_ssaoBlurFBO);
    }
    if (m_ssaoColorBufferBlur == 0)
    {
      glGenTextures(1, &m_ssaoColorBufferBlur);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
    glBindTexture(GL_TEXTURE_2D, m_ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_config.width, m_config.height, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ssaoColorBufferBlur, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      std::cerr << "[SSAORenderer] SSAO blur framebuffer incomplete" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  void SSAORenderer::CreateQuad()
  {
    if (m_quadVAO != 0)
    {
      return;
    }

    float quadVertices[] = {
        // positions   // texcoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f};

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);
  }

  void SSAORenderer::DestroyBuffers()
  {
    if (m_gBufferFBO)
    {
      glDeleteFramebuffers(1, &m_gBufferFBO);
      m_gBufferFBO = 0;
    }
    if (m_gPosition)
    {
      glDeleteTextures(1, &m_gPosition);
      m_gPosition = 0;
    }
    if (m_gNormal)
    {
      glDeleteTextures(1, &m_gNormal);
      m_gNormal = 0;
    }
    if (m_depthRBO)
    {
      glDeleteRenderbuffers(1, &m_depthRBO);
      m_depthRBO = 0;
    }
    if (m_ssaoFBO)
    {
      glDeleteFramebuffers(1, &m_ssaoFBO);
      m_ssaoFBO = 0;
    }
    if (m_ssaoColorBuffer)
    {
      glDeleteTextures(1, &m_ssaoColorBuffer);
      m_ssaoColorBuffer = 0;
    }
    if (m_ssaoBlurFBO)
    {
      glDeleteFramebuffers(1, &m_ssaoBlurFBO);
      m_ssaoBlurFBO = 0;
    }
    if (m_ssaoColorBufferBlur)
    {
      glDeleteTextures(1, &m_ssaoColorBufferBlur);
      m_ssaoColorBufferBlur = 0;
    }
    if (m_noiseTexture)
    {
      glDeleteTextures(1, &m_noiseTexture);
      m_noiseTexture = 0;
    }
    if (m_quadVAO)
    {
      glDeleteVertexArrays(1, &m_quadVAO);
      m_quadVAO = 0;
    }
    if (m_quadVBO)
    {
      glDeleteBuffers(1, &m_quadVBO);
      m_quadVBO = 0;
    }
  }

} // namespace mecha


