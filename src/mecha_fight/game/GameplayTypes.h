#pragma once

#include <functional>

#include <glm/glm.hpp>

namespace mecha
{

  struct TerrainHeightSampler
  {
    std::function<float(float, float)> callback;
    float operator()(float x, float z) const
    {
      return callback ? callback(x, z) : 0.0f;
    }
  };

  struct ThrusterParticle
  {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life{0.0f};
    float maxLife{0.0f};
    float seed{0.0f};
    float intensity{1.0f};
    float radiusScale{1.0f};
  };

  struct DashParticle
  {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life{0.0f};
    float maxLife{0.0f};
  };

  struct AfterimageParticle
  {
    glm::vec3 pos{0.0f};
    float life{0.0f};
    float maxLife{0.0f};
    float radiusScale{1.0f};
    float intensity{1.0f};
  };

  struct SparkParticle
  {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life{0.0f};
    float maxLife{0.5f};
    float seed{0.0f};
  };

  struct ShockwaveParticle
  {
    glm::vec3 center{0.0f};
    float radius{0.0f};
    float thickness{1.0f};
    float maxRadius{30.0f};
    float expansionSpeed{10.0f};
    float life{0.0f};
    float maxLife{1.0f};
    float damagePerSecond{25.0f};
    bool active{false};
    glm::vec3 color{0.3f, 0.9f, 0.9f}; // Default cyan color, can be customized
  };

  struct Bullet
  {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life{0.0f};
    bool fromEnemy{false};
    float size{0.08f}; // Size for enemy bullets, 0.06f for player bullets
  };

  struct Missile
  {
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    glm::vec3 targetPos{0.0f};
    class Enemy *target{nullptr};
    float life{0.0f};
    float maxLife{10.0f};
    bool active{false};
    bool hasTarget{false};
    float thrusterAccumulator{0.0f};
    void* soundHandle_{nullptr}; // Handle for looping rocket engine sound
    float scale{1.0f};           // Scale factor (1.0 = normal, 0.6 = mini missile)
    float damage{45.0f};         // Direct hit damage
  };

} // namespace mecha
