#pragma once

#include "../../core/Entity.h"
#include "ISoundController.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace mecha
{

  /**
   * @brief Manages 3D positional sounds with proximity-based volume attenuation
   * 
   * Tracks active 3D sounds and applies linear distance attenuation based on
   * the distance from the listener (player/camera). Follows the Entity pattern
   * for integration with the game's update loop.
   */
  class ProximitySoundSystem : public Entity
  {
  public:
    /**
     * @brief Information about a tracked 3D sound
     */
    struct SoundInfo
    {
      void* soundHandle{nullptr};        // Handle from ISoundController
      glm::vec3 position{0.0f};          // Current world position
      float maxDistance{50.0f};          // Distance at which volume reaches 0
      float baseVolume{1.0f};            // Base volume (0.0 to 1.0)
      std::string filePath;              // Sound file path (for debugging)
      bool isLooped{false};              // Whether sound is looping
    };

    ProximitySoundSystem(ISoundController* soundController);
    virtual ~ProximitySoundSystem() = default;

    /**
     * @brief Register a 3D sound to be managed by the proximity system
     * @param filePath Path to sound file
     * @param position Initial world position
     * @param looped Whether sound should loop
     * @param baseVolume Base volume (0.0 to 1.0)
     * @param maxDistance Distance at which volume reaches 0
     * @return Handle to the sound (can be used to update position or stop)
     */
    void* RegisterSound(const char* filePath, const glm::vec3& position, bool looped = false, 
                        float baseVolume = 1.0f, float maxDistance = 50.0f);

    /**
     * @brief Update the position of a tracked sound
     * @param soundHandle Handle returned from RegisterSound
     * @param newPosition New world position
     */
    void UpdateSoundPosition(void* soundHandle, const glm::vec3& newPosition);

    /**
     * @brief Stop and remove a tracked sound
     * @param soundHandle Handle returned from RegisterSound
     */
    void UnregisterSound(void* soundHandle);

    /**
     * @brief Set the listener position (player/camera)
     * @param position Listener position in world space
     * @param forward Forward direction vector (normalized)
     * @param up Up direction vector (normalized)
     */
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief Update the system - calculates distances and updates volumes
     */
    void Update(const UpdateContext& ctx) override;

  private:
    ISoundController* soundController_;
    std::vector<SoundInfo> activeSounds_;
    glm::vec3 listenerPosition_{0.0f};
    glm::vec3 listenerForward_{0.0f, 0.0f, -1.0f};
    glm::vec3 listenerUp_{0.0f, 1.0f, 0.0f};

    /**
     * @brief Calculate volume based on linear distance attenuation
     * @param distance Distance from listener to sound source
     * @param maxDistance Maximum distance (volume = 0 at this distance)
     * @param baseVolume Base volume multiplier
     * @return Calculated volume (0.0 to baseVolume)
     */
    static float CalculateVolume(float distance, float maxDistance, float baseVolume);

    /**
     * @brief Find sound info by handle
     * @param soundHandle Handle to find
     * @return Pointer to SoundInfo or nullptr if not found
     */
    SoundInfo* FindSoundInfo(void* soundHandle);

    /**
     * @brief Remove finished sounds from tracking
     */
    void CleanupFinishedSounds();
  };

} // namespace mecha

