#include "TerrainPlaceholder.h"

#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <glm/common.hpp>
#include <learnopengl/model.h>
#include <learnopengl/mesh.h>

namespace mecha
{

  TerrainMeshHandle CreateTerrainPlaceholder(const TerrainConfig &config)
  {
    TerrainMeshHandle handle{};

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const int gridSize = config.gridSize;
    const float worldScale = config.worldScale;
    const float heightScale = config.heightScale;

    // Reserve space for efficiency
    const int vertexCount = (gridSize + 1) * (gridSize + 1);
    const int indexCount = gridSize * gridSize * 6;
    vertices.reserve(vertexCount * 8); // pos(3) + normal(3) + texCoord(2)
    indices.reserve(indexCount);

    // Generate vertices with positions, normals, and texture coordinates
    for (int z = 0; z <= gridSize; ++z)
    {
      for (int x = 0; x <= gridSize; ++x)
      {
        float xPos = (x / static_cast<float>(gridSize) - 0.5f) * worldScale;
        float zPos = (z / static_cast<float>(gridSize) - 0.5f) * worldScale;

        // Simple height variation using sine waves
        float height = std::sin(xPos * 0.1f) * std::cos(zPos * 0.1f) * heightScale;

        // Position
        vertices.push_back(xPos);
        vertices.push_back(height);
        vertices.push_back(zPos);

        // Normal (pointing up for simplicity)
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);

        // Texture coordinates
        vertices.push_back(x / static_cast<float>(gridSize));
        vertices.push_back(z / static_cast<float>(gridSize));
      }
    }

    // Generate indices for triangles
    for (int z = 0; z < gridSize; ++z)
    {
      for (int x = 0; x < gridSize; ++x)
      {
        int topLeft = z * (gridSize + 1) + x;
        int topRight = topLeft + 1;
        int bottomLeft = (z + 1) * (gridSize + 1) + x;
        int bottomRight = bottomLeft + 1;

        // First triangle
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);

        // Second triangle
        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
      }
    }

    // Create OpenGL buffers
    glGenVertexArrays(1, &handle.vao);
    glGenBuffers(1, &handle.vbo);
    glGenBuffers(1, &handle.ebo);

    glBindVertexArray(handle.vao);

    glBindBuffer(GL_ARRAY_BUFFER, handle.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    handle.indexCount = static_cast<int>(indices.size());
    return handle;
  }

  float SampleTerrainHeight(float worldX, float worldZ, const TerrainConfig &config)
  {
    if (config.heightFieldReady && config.samplesX > 1 && config.samplesZ > 1 && !config.heightSamples.empty())
    {
      float normalizedX = (worldX - config.gridOrigin.x) / config.cellSize.x;
      float normalizedZ = (worldZ - config.gridOrigin.y) / config.cellSize.y;
      normalizedX = glm::clamp(normalizedX, 0.0f, static_cast<float>(config.samplesX - 1));
      normalizedZ = glm::clamp(normalizedZ, 0.0f, static_cast<float>(config.samplesZ - 1));

      int x0 = static_cast<int>(std::floor(normalizedX));
      int z0 = static_cast<int>(std::floor(normalizedZ));
      int x1 = std::min(x0 + 1, config.samplesX - 1);
      int z1 = std::min(z0 + 1, config.samplesZ - 1);
      float tx = normalizedX - static_cast<float>(x0);
      float tz = normalizedZ - static_cast<float>(z0);

      auto sample = [&](int sx, int sz)
      {
        return config.heightSamples[sz * config.samplesX + sx];
      };

      float h00 = sample(x0, z0);
      float h10 = sample(x1, z0);
      float h01 = sample(x0, z1);
      float h11 = sample(x1, z1);
      float h0 = glm::mix(h00, h10, tx);
      float h1 = glm::mix(h01, h11, tx);
      return glm::mix(h0, h1, tz);
    }

    // Fallback to procedural surface if no heightfield exists (legacy behavior)
    float height = std::sin(worldX * 0.1f) * std::cos(worldZ * 0.1f) * config.heightScale;
    return height + config.yOffset;
  }

  void BuildHeightFieldFromModel(const Model &model, TerrainConfig &config, int samplesX, int samplesZ)
  {
    if (samplesX < 2 || samplesZ < 2)
    {
      config.heightFieldReady = false;
      return;
    }

    config.samplesX = samplesX;
    config.samplesZ = samplesZ;
    config.heightSamples.assign(samplesX * samplesZ, config.defaultHeight);

    config.gridOrigin = glm::vec2(config.boundsMin.x, config.boundsMin.z);
    glm::vec2 totalSize = glm::vec2(config.boundsMax.x - config.boundsMin.x, config.boundsMax.z - config.boundsMin.z);
    config.cellSize.x = (samplesX > 1) ? totalSize.x / static_cast<float>(samplesX - 1) : 1.0f;
    config.cellSize.y = (samplesZ > 1) ? totalSize.y / static_cast<float>(samplesZ - 1) : 1.0f;
    if (!std::isfinite(config.cellSize.x) || config.cellSize.x <= 0.0f)
    {
      config.cellSize.x = 1.0f;
    }
    if (!std::isfinite(config.cellSize.y) || config.cellSize.y <= 0.0f)
    {
      config.cellSize.y = 1.0f;
    }

    auto worldToGrid = [&](const glm::vec3 &worldPos)
    {
      float fx = (worldPos.x - config.gridOrigin.x) / config.cellSize.x;
      float fz = (worldPos.z - config.gridOrigin.y) / config.cellSize.y;
      return glm::vec2(fx, fz);
    };

    auto pointInsideTriangle = [](const glm::vec2 &p, const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &c)
    {
      auto edge = [](const glm::vec2 &u, const glm::vec2 &v, const glm::vec2 &w)
      {
        return (w.x - u.x) * (v.y - u.y) - (w.y - u.y) * (v.x - u.x);
      };
      bool b1 = edge(p, a, b) >= 0.0f;
      bool b2 = edge(p, b, c) >= 0.0f;
      bool b3 = edge(p, c, a) >= 0.0f;
      return (b1 == b2) && (b2 == b3);
    };

    auto samplePlaneHeight = [](const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, float x, float z)
    {
      glm::vec3 v0 = p1 - p0;
      glm::vec3 v1 = p2 - p0;
      glm::vec3 normal = glm::cross(v0, v1);
      float denom = normal.y;
      if (std::abs(denom) < 1e-6f)
      {
        return p0.y;
      }
      float d = glm::dot(normal, p0);
      return (d - normal.x * x - normal.z * z) / denom;
    };

    auto updateCell = [&](int gx, int gz, float height)
    {
      if (gx < 0 || gx >= samplesX || gz < 0 || gz >= samplesZ)
      {
        return;
      }
      int idx = gz * samplesX + gx;
      config.heightSamples[idx] = std::max(config.heightSamples[idx], height);
    };

    for (const auto &mesh : model.meshes)
    {
      const auto &indices = mesh.indices;
      for (size_t tri = 0; tri + 2 < indices.size(); tri += 3)
      {
        const Vertex &v0 = mesh.vertices[indices[tri]];
        const Vertex &v1 = mesh.vertices[indices[tri + 1]];
        const Vertex &v2 = mesh.vertices[indices[tri + 2]];

        glm::vec3 p0 = v0.Position * config.modelScale + config.modelTranslation;
        glm::vec3 p1 = v1.Position * config.modelScale + config.modelTranslation;
        glm::vec3 p2 = v2.Position * config.modelScale + config.modelTranslation;

        glm::vec2 g0 = worldToGrid(p0);
        glm::vec2 g1 = worldToGrid(p1);
        glm::vec2 g2 = worldToGrid(p2);

        float minX = std::floor(std::min({g0.x, g1.x, g2.x}));
        float maxX = std::ceil(std::max({g0.x, g1.x, g2.x}));
        float minZ = std::floor(std::min({g0.y, g1.y, g2.y}));
        float maxZ = std::ceil(std::max({g0.y, g1.y, g2.y}));

        for (int gz = static_cast<int>(minZ); gz <= static_cast<int>(maxZ); ++gz)
        {
          for (int gx = static_cast<int>(minX); gx <= static_cast<int>(maxX); ++gx)
          {
            glm::vec2 cellCenter(static_cast<float>(gx), static_cast<float>(gz));
            if (!pointInsideTriangle(cellCenter, g0, g1, g2))
            {
              continue;
            }

            float worldX = config.gridOrigin.x + cellCenter.x * config.cellSize.x;
            float worldZ = config.gridOrigin.y + cellCenter.y * config.cellSize.y;
            float height = samplePlaneHeight(p0, p1, p2, worldX, worldZ);
            if (std::isfinite(height))
            {
              updateCell(gx, gz, height);
            }
          }
        }
      }
    }

    // Fill remaining cells (e.g., outside mesh footprint) via neighbor averaging
    for (int z = 0; z < samplesZ; ++z)
    {
      for (int x = 0; x < samplesX; ++x)
      {
        int idx = z * samplesX + x;
        if (config.heightSamples[idx] != config.defaultHeight)
        {
          continue;
        }

        float accum = 0.0f;
        int count = 0;
        for (int dz = -1; dz <= 1; ++dz)
        {
          for (int dx = -1; dx <= 1; ++dx)
          {
            if (dx == 0 && dz == 0)
              continue;
            int sx = x + dx;
            int sz = z + dz;
            if (sx >= 0 && sx < samplesX && sz >= 0 && sz < samplesZ)
            {
              int neighborIdx = sz * samplesX + sx;
              if (config.heightSamples[neighborIdx] != config.defaultHeight)
              {
                accum += config.heightSamples[neighborIdx];
                ++count;
              }
            }
          }
        }
        if (count > 0)
        {
          config.heightSamples[idx] = accum / static_cast<float>(count);
        }
      }
    }

    config.heightFieldReady = true;
  }

} // namespace mecha
