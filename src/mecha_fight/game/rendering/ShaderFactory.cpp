#include "ShaderFactory.h"
#include <iostream>

namespace mecha
{

  Shader *ShaderFactory::LoadShader(const std::string &name, const std::string &vertexPath, const std::string &fragmentPath)
  {
    // Check if already loaded
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
      std::cout << "[ShaderFactory] Shader '" << name << "' already loaded, returning cached version" << std::endl;
      return it->second.get();
    }

    // Load new shader
    try
    {
      auto shader = std::make_unique<Shader>(vertexPath.c_str(), fragmentPath.c_str());
      m_shaderPaths[name] = {vertexPath, fragmentPath};

      Shader *ptr = shader.get();
      m_shaders[name] = std::move(shader);

      std::cout << "[ShaderFactory] Loaded shader '" << name << "'" << std::endl;
      return ptr;
    }
    catch (const std::exception &e)
    {
      std::cout << "[ShaderFactory] Failed to load shader '" << name << "': " << e.what() << std::endl;
      return nullptr;
    }
  }

  Shader *ShaderFactory::GetShader(const std::string &name)
  {
    auto it = m_shaders.find(name);
    if (it != m_shaders.end())
    {
      return it->second.get();
    }

    std::cout << "[ShaderFactory] Shader '" << name << "' not found" << std::endl;
    return nullptr;
  }

  bool ShaderFactory::HasShader(const std::string &name) const
  {
    return m_shaders.find(name) != m_shaders.end();
  }

  bool ShaderFactory::ReloadShader(const std::string &name)
  {
    auto pathIt = m_shaderPaths.find(name);
    if (pathIt == m_shaderPaths.end())
    {
      std::cout << "[ShaderFactory] Cannot reload '" << name << "' - paths not found" << std::endl;
      return false;
    }

    try
    {
      const auto &[vertexPath, fragmentPath] = pathIt->second;
      auto shader = std::make_unique<Shader>(vertexPath.c_str(), fragmentPath.c_str());
      m_shaders[name] = std::move(shader);

      std::cout << "[ShaderFactory] Reloaded shader '" << name << "'" << std::endl;
      return true;
    }
    catch (const std::exception &e)
    {
      std::cout << "[ShaderFactory] Failed to reload shader '" << name << "': " << e.what() << std::endl;
      return false;
    }
  }

  void ShaderFactory::Clear()
  {
    m_shaders.clear();
    m_shaderPaths.clear();
    std::cout << "[ShaderFactory] Cleared all shaders" << std::endl;
  }

} // namespace mecha
