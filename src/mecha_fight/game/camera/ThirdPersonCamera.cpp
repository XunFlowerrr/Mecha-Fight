#include "ThirdPersonCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace mecha
{

  ThirdPersonCamera::ThirdPersonCamera()
      : m_camera(glm::vec3(0.0f, 2.0f, 5.0f)),
        m_yaw(kDefaultYaw),
        m_pitch(kDefaultPitch),
        m_nearPlane(kDefaultNearPlane),
        m_lastMouseX(0.0f),
        m_lastMouseY(0.0f),
        m_firstMouse(true),
        m_terrainSampler{}
  {
  }

  void ThirdPersonCamera::Update(const glm::vec3 &targetPos, float distance, float heightOffset)
  {
    // Calculate camera position in orbital sphere around target
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    glm::vec3 cameraOffset(
        distance * cos(pitchRad) * sin(yawRad),
        distance * sin(pitchRad),
        distance * cos(pitchRad) * cos(yawRad));

    glm::vec3 cameraTarget = targetPos + glm::vec3(0.0f, heightOffset, 0.0f);
    glm::vec3 cameraPosition = cameraTarget + cameraOffset;

    // Check for terrain collision
    float collisionDistance = CheckTerrainCollision(cameraPosition, cameraTarget, distance);

    // Adjust camera if collision detected
    if (collisionDistance < distance)
    {
      cameraOffset = glm::normalize(cameraOffset) * collisionDistance;
      cameraPosition = cameraTarget + cameraOffset;

      // Use dynamic near-plane clipping for very close positions
      if (collisionDistance < kMinDistance * 2.0f)
      {
        m_nearPlane = std::max(0.01f, collisionDistance * 0.5f);
      }
      else
      {
        m_nearPlane = kDefaultNearPlane;
      }
    }
    else
    {
      m_nearPlane = kDefaultNearPlane;
    }

    m_camera.Position = cameraPosition;
    m_camera.Front = glm::normalize(cameraTarget - cameraPosition);
  }

  void ThirdPersonCamera::ProcessMouseMovement(float xOffset, float yOffset)
  {
    m_yaw -= xOffset * kMouseSensitivity;
    m_pitch -= yOffset * kMouseSensitivity;

    // Clamp pitch to prevent flipping
    m_pitch = glm::clamp(m_pitch, kMinPitch, kMaxPitch);
  }

  void ThirdPersonCamera::ResetMouseTracking(float screenCenterX, float screenCenterY)
  {
    m_firstMouse = true;
    m_lastMouseX = screenCenterX;
    m_lastMouseY = screenCenterY;
  }

  float ThirdPersonCamera::CheckTerrainCollision(const glm::vec3 &cameraPos, const glm::vec3 &targetPos, float desiredDistance)
  {
    if (!m_terrainSampler.callback)
    {
      return desiredDistance;
    }

    // Raycast from target towards camera
    glm::vec3 rayDir = glm::normalize(cameraPos - targetPos);
    float currentDist = 0.0f;

    while (currentDist < desiredDistance)
    {
      glm::vec3 checkPos = targetPos + rayDir * currentDist;
      float terrainHeight = m_terrainSampler.callback(checkPos.x, checkPos.z);

      // If camera ray goes below terrain, collision detected
      if (checkPos.y < terrainHeight + kCollisionOffset)
      {
        return currentDist;
      }

      currentDist += kCollisionStepSize;
    }

    return desiredDistance;
  }

} // namespace mecha
