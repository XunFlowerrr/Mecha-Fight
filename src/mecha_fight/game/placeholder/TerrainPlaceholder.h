#pragma once

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

class Model;

namespace mecha
{

  // Simple terrain mesh handle for placeholder geometry (currently a procedural sine-wave terrain).
  struct TerrainMeshHandle
  {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int indexCount = 0;
  };

  // Configuration for procedural terrain generation
  struct TerrainConfig
  {
    int gridSize = 200;        // Number of grid cells
    float worldScale = 500.0f; // Physical world size
    float heightScale = 2.0f;  // Height variation amplitude
    float yOffset = -3.0f;     // Vertical offset of terrain

    // Model-driven terrain data
    Model *terrainModel{nullptr};
    glm::vec3 modelScale{1.0f};
    glm::vec3 modelTranslation{0.0f};
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    glm::vec2 gridOrigin{0.0f};
    glm::vec2 cellSize{1.0f, 1.0f};
    int samplesX{0};
    int samplesZ{0};
    float defaultHeight{-3.0f};
    bool heightFieldReady{false};
    std::vector<float> heightSamples;
  };

  // Creates a placeholder procedural terrain mesh until a terrain model is integrated.
  // Uses sine wave height variation for simple rolling hills effect.
  TerrainMeshHandle CreateTerrainPlaceholder(const TerrainConfig &config = TerrainConfig{});

  // Height sampling function matching the generated terrain
  // Used for collision detection and camera terrain avoidance
  float SampleTerrainHeight(float worldX, float worldZ, const TerrainConfig &config = TerrainConfig{});

  // Builds a heightfield lookup from a static model to enable collision sampling
  void BuildHeightFieldFromModel(const Model &model, TerrainConfig &config, int samplesX = 256, int samplesZ = 256);

} // namespace mecha
