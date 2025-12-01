#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace mecha
{

  class SSAORenderer
  {
  public:
    struct Config
    {
      unsigned int width = 0;
      unsigned int height = 0;
      unsigned int kernelSize = 64;
    };

    SSAORenderer();
    ~SSAORenderer();

    bool Init(const Config &config);
    void Resize(unsigned int width, unsigned int height);
    void Cleanup();

    void BeginGeometryPass();
    void EndGeometryPass();

    unsigned int GetPositionTexture() const { return m_gPosition; }
    unsigned int GetNormalTexture() const { return m_gNormal; }
    unsigned int GetSSAORawTexture() const { return m_ssaoColorBuffer; }
    unsigned int GetSSAOBlurTexture() const { return m_ssaoColorBufferBlur; }
    unsigned int GetNoiseTexture() const { return m_noiseTexture; }
    unsigned int GetSSAOFBO() const { return m_ssaoFBO; }
    unsigned int GetSSAOBlurFBO() const { return m_ssaoBlurFBO; }
    unsigned int GetQuadVAO() const { return m_quadVAO; }
    const std::vector<glm::vec3> &GetKernel() const { return m_kernel; }
    bool IsInitialized() const { return m_initialized; }
    unsigned int GetWidth() const { return m_config.width; }
    unsigned int GetHeight() const { return m_config.height; }

  private:
    void GenerateKernel();
    void CreateNoiseTexture();
    void CreateGBuffer();
    void CreateSSAOBuffers();
    void CreateQuad();
    void DestroyBuffers();

    Config m_config{};
    bool m_initialized{false};

    unsigned int m_gBufferFBO{0};
    unsigned int m_gPosition{0};
    unsigned int m_gNormal{0};
    unsigned int m_depthRBO{0};

    unsigned int m_ssaoFBO{0};
    unsigned int m_ssaoColorBuffer{0};

    unsigned int m_ssaoBlurFBO{0};
    unsigned int m_ssaoColorBufferBlur{0};

    unsigned int m_noiseTexture{0};

    unsigned int m_quadVAO{0};
    unsigned int m_quadVBO{0};

    std::vector<glm::vec3> m_kernel;
  };

} // namespace mecha


