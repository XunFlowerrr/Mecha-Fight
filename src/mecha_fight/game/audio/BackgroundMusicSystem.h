#pragma once

#include "ISoundController.h"
#include <string>
#include <vector>

namespace mecha
{

  /**
   * @brief Background music system with stage-based music and crossfade transitions
   *
   * Manages background music playback with support for:
   * - Multiple music stages (normal, boss fight, etc.)
   * - Crossfade transitions between stages
   * - Multiple tracks per stage with sequential playback
   * - Fade out on specific events (boss death)
   */
  class BackgroundMusicSystem
  {
  public:
    enum class MusicStage
    {
      None,
      Normal,
      BossFight
    };

    BackgroundMusicSystem(ISoundController *soundController);
    ~BackgroundMusicSystem() = default;

    /**
     * @brief Initialize the music system and preload tracks
     */
    void Initialize();

    /**
     * @brief Update the music system (handles fading and track transitions)
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    /**
     * @brief Set the current music stage with crossfade
     * @param stage The new music stage
     * @param fadeDuration Duration of the crossfade in seconds
     */
    void SetStage(MusicStage stage, float fadeDuration = 2.0f);

    /**
     * @brief Fade out current music (e.g., when boss dies)
     * @param fadeDuration Duration of the fade out in seconds
     */
    void FadeOut(float fadeDuration = 3.0f);

    /**
     * @brief Stop all music immediately
     */
    void Stop();

    /**
     * @brief Set the music volume
     * @param volume Volume level (0.0 to 1.0)
     */
    void SetVolume(float volume);

    /**
     * @brief Get the current music volume
     * @return Current volume level
     */
    float GetVolume() const { return baseVolume_; }

    /**
     * @brief Get the current music stage
     */
    MusicStage GetCurrentStage() const { return currentStage_; }

    /**
     * @brief Check if music is currently fading
     */
    bool IsFading() const { return isFading_; }

  private:
    ISoundController *soundController_;

    // Current state
    MusicStage currentStage_{MusicStage::None};
    MusicStage targetStage_{MusicStage::None};
    void *currentTrackHandle_{nullptr};
    float baseVolume_{0.5f};
    float currentVolume_{0.0f};

    // Track management
    std::vector<std::string> normalTracks_;
    std::vector<std::string> bossFightTracks_;
    size_t currentTrackIndex_{0};

    // Fading state
    bool isFading_{false};
    bool isFadingOut_{false};
    float fadeTimer_{0.0f};
    float fadeDuration_{2.0f};
    float fadeStartVolume_{0.0f};
    float fadeTargetVolume_{0.0f};

    /**
     * @brief Play the next track for the current stage
     */
    void PlayNextTrack();

    /**
     * @brief Get the tracks for a given stage
     */
    const std::vector<std::string> &GetTracksForStage(MusicStage stage) const;

    /**
     * @brief Start a fade transition
     */
    void StartFade(float targetVolume, float duration);

    /**
     * @brief Check if current track has finished and play next
     */
    void CheckTrackCompletion();
  };

} // namespace mecha
