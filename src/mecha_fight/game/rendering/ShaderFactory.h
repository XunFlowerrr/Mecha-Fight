#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <learnopengl/shader_m.h>

namespace mecha
{

  /**
   * @brief Factory for creating and caching shader programs
   *
   * Handles shader compilation, linking, and caching. Supports hot-reloading
   * and provides error handling for shader compilation failures.
   */
  class ShaderFactory
  {
  public:
    /**
     * @brief Load or retrieve cached shader
     * @param name Unique identifier for the shader
     * @param vertexPath Path to vertex shader source
     * @param fragmentPath Path to fragment shader source
     * @return Pointer to compiled shader, nullptr on failure
     */
    Shader *LoadShader(const std::string &name, const std::string &vertexPath, const std::string &fragmentPath);

    /**
     * @brief Get previously loaded shader
     * @param name Shader identifier
     * @return Pointer to shader, nullptr if not found
     */
    Shader *GetShader(const std::string &name);

    /**
     * @brief Check if shader is cached
     */
    bool HasShader(const std::string &name) const;

    /**
     * @brief Reload shader from disk (hot-reload support)
     * @param name Shader identifier
     * @return true if reload succeeded
     */
    bool ReloadShader(const std::string &name);

    /**
     * @brief Clear all cached shaders
     */
    void Clear();

    /**
     * @brief Get number of loaded shaders
     */
    size_t GetLoadedCount() const { return m_shaders.size(); }

  private:
    std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;

    // Store paths for hot-reloading
    std::unordered_map<std::string, std::pair<std::string, std::string>> m_shaderPaths;
  };

} // namespace mecha
