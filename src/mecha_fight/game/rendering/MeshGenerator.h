#pragma once

#include <unordered_map>
#include <string>
#include "../placeholder/TerrainPlaceholder.h"
#include "../placeholder/EnemyPlaceholder.h"

namespace mecha
{

  /**
   * @brief Factory for generating procedural meshes
   *
   * Creates and caches procedural geometry like terrain, spheres, and other primitives.
   * Useful for placeholder meshes and debug visualization.
   */
  class MeshGenerator
  {
  public:
    ~MeshGenerator();

    /**
     * @brief Generate or retrieve cached sphere mesh
     * @param name Unique identifier for the sphere
     * @param stacks Number of horizontal subdivisions
     * @param slices Number of vertical subdivisions
     * @return Handle to sphere mesh
     */
    MeshHandle GenerateSphere(const std::string &name, int stacks = 16, int slices = 24);

    /**
     * @brief Generate or retrieve cached terrain mesh
     * @param name Unique identifier for the terrain
     * @param config Terrain generation parameters
     * @return Handle to terrain mesh
     */
    TerrainMeshHandle GenerateTerrain(const std::string &name, const TerrainConfig &config);

    /**
     * @brief Get previously generated mesh
     * @param name Mesh identifier
     * @return Pointer to mesh handle, nullptr if not found
     */
    const MeshHandle *GetSphere(const std::string &name) const;

    /**
     * @brief Get previously generated terrain
     * @param name Terrain identifier
     * @return Pointer to terrain mesh handle, nullptr if not found
     */
    const TerrainMeshHandle *GetTerrain(const std::string &name) const;

    /**
     * @brief Check if mesh exists
     */
    bool HasSphere(const std::string &name) const;
    bool HasTerrain(const std::string &name) const;

    /**
     * @brief Clear all cached meshes (frees GPU memory)
     */
    void Clear();

  private:
    std::unordered_map<std::string, MeshHandle> m_spheres;
    std::unordered_map<std::string, TerrainMeshHandle> m_terrains;

    void CleanupSpheres();
    void CleanupTerrains();
  };

} // namespace mecha
