#pragma once

#include "ISoundController.h"
#include "ProximitySoundSystem.h"
#include "SoundRegistry.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace mecha
{

  /**
   * @brief High-level sound manager that provides easy sound playback interface
   *
   * Wraps ISoundController and ProximitySoundSystem to provide a simple API
   * for playing sounds by name. Handles sound registration, 3D positioning,
   * and proximity-based volume attenuation automatically.
   *
   * Entities don't need to know about file paths or sound system internals.
   */
  class SoundManager
  {
  public:
    /**
     * @brief Sound configuration for registered sounds
     */
    struct SoundConfig
    {
      const char* filePath{nullptr};
      float defaultVolume{1.0f};
      float maxDistance{50.0f};
      bool isLooped{false};
      float minInterval{0.0f}; // Minimum seconds between plays (global)
    };

    SoundManager(ISoundController* soundController);
    ~SoundManager() = default;

    /**
     * @brief Register a sound with a name for easy playback
     * @param name Unique name identifier (e.g., "PLAYER_SHOOT")
     * @param config Sound configuration
     */
    void RegisterSound(const std::string& name, const SoundConfig& config);

    /**
     * @brief Play a 3D positional sound by name
     * @param name Sound name (must be registered)
     * @param position World position of the sound
     * @param volumeOverride Optional volume override (uses default if not provided)
     * @param maxDistanceOverride Optional max distance override
     * @return Handle to the sound (nullptr if sound not found or failed to play)
     */
    void* PlaySound3D(const std::string& name, const glm::vec3& position,
                     float volumeOverride = -1.0f, float maxDistanceOverride = -1.0f);

    /**
     * @brief Play a 2D sound by name (not positional, for UI/music)
     * @param name Sound name (must be registered)
     * @param volumeOverride Optional volume override
     * @return Handle to the sound
     */
    void* PlaySound2D(const std::string& name, float volumeOverride = -1.0f);

    /**
     * @brief Update the position of a 3D sound
     * @param soundHandle Handle returned from PlaySound3D
     * @param newPosition New world position
     */
    void UpdateSoundPosition(void* soundHandle, const glm::vec3& newPosition);

    /**
     * @brief Set the pitch/playback speed of a sound
     * @param soundHandle Handle returned from PlaySound3D or PlaySound2D
     * @param pitch Pitch multiplier (1.0 = normal, >1.0 = faster, <1.0 = slower)
     */
    void SetSoundPitch(void* soundHandle, float pitch);

    /**
     * @brief Stop a playing sound
     * @param soundHandle Handle returned from PlaySound3D or PlaySound2D
     */
    void StopSound(void* soundHandle);

    /**
     * @brief Set the listener position (player/camera)
     * @param position Listener position
     * @param forward Forward direction vector
     * @param up Up direction vector
     */
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief Update the sound system (call each frame)
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Preload a sound by name to avoid runtime IO
     */
    void PreloadSound(const std::string& name);

    /**
     * @brief Set the master volume for all sounds
     * @param volume Master volume (0.0 to 1.0, where 1.0 is full volume)
     */
    void SetMasterVolume(float volume);

    /**
     * @brief Get the current master volume
     * @return Master volume (0.0 to 1.0)
     */
    float GetMasterVolume() const;

    /**
     * @brief Set the volume multiplier for a specific sound
     * @param name Sound name (must be registered)
     * @param volume Volume multiplier (0.0 to 1.0, where 1.0 is full volume)
     */
    void SetSoundVolume(const std::string& name, float volume);

    /**
     * @brief Get the volume multiplier for a specific sound
     * @param name Sound name (must be registered)
     * @return Volume multiplier (0.0 to 1.0), or 1.0 if not set
     */
    float GetSoundVolume(const std::string& name) const;

    /**
     * @brief Get the underlying proximity sound system (for advanced use)
     */
    std::shared_ptr<ProximitySoundSystem> GetProximitySystem() { return proximitySystem_; }

    /**
     * @brief Get all registered sounds (read-only access)
     * @return Const reference to the map of registered sounds
     */
    const std::unordered_map<std::string, SoundConfig>& GetRegisteredSounds() const { return registeredSounds_; }

  private:
    ISoundController* soundController_;
    std::shared_ptr<ProximitySoundSystem> proximitySystem_;
    std::unordered_map<std::string, SoundConfig> registeredSounds_;
    std::unordered_map<std::string, float> lastPlayTime_;
    std::unordered_map<std::string, float> soundVolumes_; // Per-sound volume multipliers
    float elapsedTime_{0.0f};
  };

} // namespace mecha

