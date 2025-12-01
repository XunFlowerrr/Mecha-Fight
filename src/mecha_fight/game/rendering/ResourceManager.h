#pragma once

#include "ShaderFactory.h"
#include "ModelLoader.h"
#include "MeshGenerator.h"
#include <glad/glad.h>
#include <string>
#include <memory>

namespace mecha
{
  // Forward declarations
  class ISoundController;
  class SoundManager;

  /**
   * @brief Central manager for all game rendering resources
   *
   * Provides unified access to factories and handles common resources like UI geometry.
   * Simplifies resource initialization and cleanup.
   */
  class ResourceManager
  {
  public:
    ResourceManager();
    ~ResourceManager();

    // Non-copyable
    ResourceManager(const ResourceManager &) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;

    /**
     * @brief Initialize common game resources
     * @return true if initialization succeeded
     */
    bool Initialize();

    /**
     * @brief Cleanup all resources
     */
    void Shutdown();

    // Factory accessors
    ShaderFactory &Shaders() { return m_shaderFactory; }
    ModelLoader &Models() { return m_modelLoader; }
    MeshGenerator &Meshes() { return m_meshGenerator; }

    const ShaderFactory &Shaders() const { return m_shaderFactory; }
    const ModelLoader &Models() const { return m_modelLoader; }
    const MeshGenerator &Meshes() const { return m_meshGenerator; }

    // UI quad geometry
    unsigned int GetUIQuadVAO() const { return m_uiQuadVAO; }
    unsigned int GetSkyboxCubemap() const { return m_skyboxCubemap; }
    bool LoadSkyboxCubemap(const std::string &path);

    // Sound system access
    void SetSoundManager(SoundManager* soundManager) { m_soundManager = soundManager; }
    SoundManager* GetSoundManager() { return m_soundManager; }
    const SoundManager* GetSoundManager() const { return m_soundManager; }
    
    // Legacy access (for advanced use only)
    void SetSoundController(ISoundController* soundController) { m_soundController = soundController; }
    ISoundController* GetSoundController() { return m_soundController; }
    const ISoundController* GetSoundController() const { return m_soundController; }

  private:
    ShaderFactory m_shaderFactory;
    ModelLoader m_modelLoader;
    MeshGenerator m_meshGenerator;

    // Common UI geometry
    unsigned int m_uiQuadVAO;
    unsigned int m_uiQuadVBO;
    unsigned int m_skyboxCubemap;

    // Sound system
    SoundManager* m_soundManager{nullptr};
    ISoundController* m_soundController{nullptr}; // Kept for legacy/advanced access

    void CreateUIQuad();
    void CleanupUIQuad();
  };

} // namespace mecha
