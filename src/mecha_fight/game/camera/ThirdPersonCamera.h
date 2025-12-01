#pragma once

#include <glm/glm.hpp>
#include <learnopengl/camera.h>

namespace mecha
{

  class MechaPlayer;

  // Third-person camera that orbits around a target with collision detection
  class ThirdPersonCamera
  {
  public:
    static constexpr float kDefaultYaw = 0.0f;
    static constexpr float kDefaultPitch = 20.0f;
    static constexpr float kMinPitch = -30.0f;
    static constexpr float kMaxPitch = 60.0f;
    static constexpr float kMouseSensitivity = 0.3f;
    static constexpr float kMinDistance = 0.5f;
    static constexpr float kCollisionOffset = 0.1f;
    static constexpr float kCollisionStepSize = 0.05f;
    static constexpr float kDefaultNearPlane = 0.1f;

    struct TerrainSampler
    {
      float (*callback)(float x, float z) = nullptr;
    };

    ThirdPersonCamera();

    // Update camera based on target position and configuration
    void Update(const glm::vec3 &targetPos, float distance, float heightOffset);

    // Handle mouse movement for camera rotation
    void ProcessMouseMovement(float xOffset, float yOffset);

    // Reset mouse position tracking (call when capturing cursor)
    void ResetMouseTracking(float screenCenterX, float screenCenterY);

    // Get the underlying Camera object for rendering
    Camera &GetCamera() { return m_camera; }
    const Camera &GetCamera() const { return m_camera; }

    // Get camera yaw (for syncing player rotation)
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    // Get dynamic near plane (for clipping through terrain)
    float GetNearPlane() const { return m_nearPlane; }

    // Set terrain height sampler for collision detection
    void SetTerrainSampler(TerrainSampler sampler) { m_terrainSampler = sampler; }

  private:
    float CheckTerrainCollision(const glm::vec3 &cameraPos, const glm::vec3 &targetPos, float desiredDistance);

    Camera m_camera;
    float m_yaw;
    float m_pitch;
    float m_nearPlane;
    float m_lastMouseX;
    float m_lastMouseY;
    bool m_firstMouse;
    TerrainSampler m_terrainSampler;
  };

} // namespace mecha
