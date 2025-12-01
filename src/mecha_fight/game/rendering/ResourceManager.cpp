#include "ResourceManager.h"
#include "SkyboxLoader.h"
#include <iostream>

namespace mecha
{

  ResourceManager::ResourceManager()
      : m_uiQuadVAO(0), m_uiQuadVBO(0), m_skyboxCubemap(0)
  {
  }

  ResourceManager::~ResourceManager()
  {
    Shutdown();
  }

  bool ResourceManager::Initialize()
  {
    std::cout << "[ResourceManager] Initializing..." << std::endl;

    // Create UI quad geometry
    CreateUIQuad();

    std::cout << "[ResourceManager] Initialization complete" << std::endl;
    return true;
  }

  void ResourceManager::Shutdown()
  {
    std::cout << "[ResourceManager] Shutting down..." << std::endl;

    CleanupUIQuad();

    if (m_skyboxCubemap != 0)
    {
      glDeleteTextures(1, &m_skyboxCubemap);
      m_skyboxCubemap = 0;
    }

    m_meshGenerator.Clear();
    m_modelLoader.Clear();
    m_shaderFactory.Clear();

    std::cout << "[ResourceManager] Shutdown complete" << std::endl;
  }

  bool ResourceManager::LoadSkyboxCubemap(const std::string &path)
  {
    if (m_skyboxCubemap != 0)
    {
      glDeleteTextures(1, &m_skyboxCubemap);
      m_skyboxCubemap = 0;
    }

    m_skyboxCubemap = SkyboxLoader::LoadVerticalCrossCubemap(path);
    return m_skyboxCubemap != 0;
  }

  void ResourceManager::CreateUIQuad()
  {
    // UI quad (NDC via shader using screen pixels)
    float uiVerts[] = {
        // aPos (0..1)
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f};

    glGenVertexArrays(1, &m_uiQuadVAO);
    glGenBuffers(1, &m_uiQuadVBO);
    glBindVertexArray(m_uiQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_uiQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVerts), uiVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    std::cout << "[ResourceManager] Created UI quad geometry" << std::endl;
  }

  void ResourceManager::CleanupUIQuad()
  {
    if (m_uiQuadVAO != 0)
    {
      glDeleteVertexArrays(1, &m_uiQuadVAO);
      m_uiQuadVAO = 0;
    }

    if (m_uiQuadVBO != 0)
    {
      glDeleteBuffers(1, &m_uiQuadVBO);
      m_uiQuadVBO = 0;
    }
  }

} // namespace mecha
