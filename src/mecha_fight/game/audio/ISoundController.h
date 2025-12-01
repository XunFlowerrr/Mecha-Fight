#pragma once

#include <glm/glm.hpp>

namespace mecha
{

  /**
   * @brief Abstract interface for sound playback and management
   * 
   * Provides an abstraction layer for sound engines, allowing the game
   * to work with any sound implementation (irrKlang, FMOD, etc.)
   * Follows Dependency Inversion Principle - entities depend on this interface,
   * not concrete implementations.
   */
  class ISoundController
  {
  public:
    virtual ~ISoundController() = default;

    /**
     * @brief Play a 3D positional sound in the world
     * @param filePath Path to the sound file
     * @param position World position of the sound source
     * @param looped Whether the sound should loop
     * @param volume Base volume (0.0 to 1.0)
     * @return Opaque handle to the sound (can be used to stop/control it later)
     */
    virtual void* Play3D(const char* filePath, const glm::vec3& position, bool looped = false, float volume = 1.0f) = 0;

    /**
     * @brief Play a 2D sound (not positional, for UI/music)
     * @param filePath Path to the sound file
     * @param looped Whether the sound should loop
     * @param volume Volume (0.0 to 1.0)
     * @return Opaque handle to the sound
     */
    virtual void* Play2D(const char* filePath, bool looped = false, float volume = 1.0f) = 0;

    /**
     * @brief Update the listener position (player/camera)
     * @param position Listener position in world space
     * @param forward Forward direction vector (normalized)
     * @param up Up direction vector (normalized)
     */
    virtual void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) = 0;

    /**
     * @brief Stop a playing sound
     * @param soundHandle Handle returned from Play3D or Play2D
     */
    virtual void StopSound(void* soundHandle) = 0;

    /**
     * @brief Update the volume of a playing sound
     * @param soundHandle Handle returned from Play3D or Play2D
     * @param volume New volume (0.0 to 1.0)
     */
    virtual void SetVolume(void* soundHandle, float volume) = 0;

    /**
     * @brief Update the position of a 3D sound
     * @param soundHandle Handle returned from Play3D
     * @param position New world position
     */
    virtual void SetPosition(void* soundHandle, const glm::vec3& position) = 0;

    /**
     * @brief Set the pitch/playback speed of a sound
     * @param soundHandle Handle returned from Play3D or Play2D
     * @param pitch Pitch multiplier (1.0 = normal, >1.0 = faster, <1.0 = slower)
     */
    virtual void SetPitch(void* soundHandle, float pitch) = 0;

    /**
     * @brief Check if a sound is still playing
     * @param soundHandle Handle returned from Play3D or Play2D
     * @return true if sound is playing, false otherwise
     */
    virtual bool IsPlaying(void* soundHandle) const = 0;

    /**
     * @brief Update the sound engine (call each frame)
     * @param deltaTime Time since last frame
     */
    virtual void Update(float deltaTime) = 0;

    /**
     * @brief Shutdown and cleanup the sound engine
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Preload a sound file into memory to avoid runtime IO
     * @param filePath Path to the sound file
     */
    virtual void PreloadSound(const char* filePath) = 0;

    /**
     * @brief Set the master volume for all sounds
     * @param volume Master volume (0.0 to 1.0, where 1.0 is full volume)
     */
    virtual void SetMasterVolume(float volume) = 0;

    /**
     * @brief Get the current master volume
     * @return Master volume (0.0 to 1.0)
     */
    virtual float GetMasterVolume() const = 0;
  };

} // namespace mecha

