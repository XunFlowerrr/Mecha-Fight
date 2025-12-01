#include "MeshGenerator.h"
#include <glad/glad.h>
#include <iostream>

namespace mecha
{

  MeshGenerator::~MeshGenerator()
  {
    Clear();
  }

  MeshHandle MeshGenerator::GenerateSphere(const std::string &name, int stacks, int slices)
  {
    // Check if already generated
    auto it = m_spheres.find(name);
    if (it != m_spheres.end())
    {
      std::cout << "[MeshGenerator] Sphere '" << name << "' already generated, returning cached version" << std::endl;
      return it->second;
    }

    // Generate new sphere
    std::cout << "[MeshGenerator] Generating sphere '" << name << "' (stacks=" << stacks << ", slices=" << slices << ")" << std::endl;
    MeshHandle mesh = CreateEnemyPlaceholderSphere(stacks, slices);
    m_spheres[name] = mesh;

    std::cout << "[MeshGenerator] Generated sphere with " << mesh.indexCount << " indices" << std::endl;
    return mesh;
  }

  TerrainMeshHandle MeshGenerator::GenerateTerrain(const std::string &name, const TerrainConfig &config)
  {
    // Check if already generated
    auto it = m_terrains.find(name);
    if (it != m_terrains.end())
    {
      std::cout << "[MeshGenerator] Terrain '" << name << "' already generated, returning cached version" << std::endl;
      return it->second;
    }

    // Generate new terrain
    std::cout << "[MeshGenerator] Generating terrain '" << name << "'" << std::endl;
    TerrainMeshHandle mesh = CreateTerrainPlaceholder(config);
    m_terrains[name] = mesh;

    std::cout << "[MeshGenerator] Generated terrain with " << mesh.indexCount << " indices" << std::endl;
    return mesh;
  }

  const MeshHandle *MeshGenerator::GetSphere(const std::string &name) const
  {
    auto it = m_spheres.find(name);
    if (it != m_spheres.end())
    {
      return &it->second;
    }
    return nullptr;
  }

  const TerrainMeshHandle *MeshGenerator::GetTerrain(const std::string &name) const
  {
    auto it = m_terrains.find(name);
    if (it != m_terrains.end())
    {
      return &it->second;
    }
    return nullptr;
  }

  bool MeshGenerator::HasSphere(const std::string &name) const
  {
    return m_spheres.find(name) != m_spheres.end();
  }

  bool MeshGenerator::HasTerrain(const std::string &name) const
  {
    return m_terrains.find(name) != m_terrains.end();
  }

  void MeshGenerator::Clear()
  {
    CleanupSpheres();
    CleanupTerrains();
    std::cout << "[MeshGenerator] Cleared all meshes" << std::endl;
  }

  void MeshGenerator::CleanupSpheres()
  {
    for (auto &[name, mesh] : m_spheres)
    {
      if (mesh.vao != 0)
      {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
      }
    }
    m_spheres.clear();
  }

  void MeshGenerator::CleanupTerrains()
  {
    for (auto &[name, mesh] : m_terrains)
    {
      if (mesh.vao != 0)
      {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
      }
    }
    m_terrains.clear();
  }

} // namespace mecha
