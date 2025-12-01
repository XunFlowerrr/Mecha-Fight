#include "BackgroundMusicSystem.h"
#include <iostream>
#include <algorithm>

namespace mecha
{

  BackgroundMusicSystem::BackgroundMusicSystem(ISoundController *soundController)
      : soundController_(soundController)
  {
    // Define tracks for each stage
    normalTracks_ = {
        "resources/audio/music/Low-Orbit.mp3"};

    bossFightTracks_ = {
        "resources/audio/music/Requiem-of-the-Dying-Light.mp3",
        "resources/audio/music/Requiem-of-the-Dying-Light-2.mp3"};
  }

  void BackgroundMusicSystem::Initialize()
  {
    if (!soundController_)
    {
      std::cerr << "[BackgroundMusicSystem] No sound controller available!" << std::endl;
      return;
    }

    // Preload all music tracks
    for (const auto &track : normalTracks_)
    {
      soundController_->PreloadSound(track.c_str());
    }
    for (const auto &track : bossFightTracks_)
    {
      soundController_->PreloadSound(track.c_str());
    }

    std::cout << "[BackgroundMusicSystem] Initialized with "
              << normalTracks_.size() << " normal tracks and "
              << bossFightTracks_.size() << " boss fight tracks" << std::endl;
  }

  void BackgroundMusicSystem::Update(float deltaTime)
  {
    if (!soundController_)
    {
      return;
    }

    // Handle fading
    if (isFading_)
    {
      fadeTimer_ += deltaTime;
      float progress = std::min(1.0f, fadeTimer_ / fadeDuration_);

      // Smooth easing (ease in-out)
      float easedProgress = progress < 0.5f
                                ? 2.0f * progress * progress
                                : 1.0f - std::pow(-2.0f * progress + 2.0f, 2.0f) / 2.0f;

      currentVolume_ = fadeStartVolume_ + (fadeTargetVolume_ - fadeStartVolume_) * easedProgress;

      // Update track volume
      if (currentTrackHandle_)
      {
        soundController_->SetVolume(currentTrackHandle_, currentVolume_);
      }

      // Fade complete
      if (progress >= 1.0f)
      {
        isFading_ = false;
        currentVolume_ = fadeTargetVolume_;

        // If we faded out completely
        if (fadeTargetVolume_ <= 0.001f)
        {
          // Stop the current track
          if (currentTrackHandle_)
          {
            soundController_->StopSound(currentTrackHandle_);
            currentTrackHandle_ = nullptr;
          }

          // If we're transitioning to a new stage (not just fading out)
          if (!isFadingOut_ && targetStage_ != MusicStage::None)
          {
            currentStage_ = targetStage_;
            currentTrackIndex_ = 0;
            PlayNextTrack();

            // Start fade in
            StartFade(baseVolume_, fadeDuration_);
          }
          else
          {
            // Just fading out, set stage to None
            currentStage_ = MusicStage::None;
            isFadingOut_ = false;
          }
        }
      }
    }

    // Check if current track finished playing (for sequential playback)
    CheckTrackCompletion();
  }

  void BackgroundMusicSystem::SetStage(MusicStage stage, float fadeDuration)
  {
    if (stage == currentStage_ && !isFadingOut_)
    {
      return; // Already in this stage
    }

    targetStage_ = stage;
    fadeDuration_ = fadeDuration;
    isFadingOut_ = false;

    if (currentStage_ == MusicStage::None || currentTrackHandle_ == nullptr)
    {
      // No music playing, start directly
      currentStage_ = stage;
      currentTrackIndex_ = 0;
      PlayNextTrack();

      // Fade in from silence
      currentVolume_ = 0.0f;
      if (currentTrackHandle_)
      {
        soundController_->SetVolume(currentTrackHandle_, 0.0f);
      }
      StartFade(baseVolume_, fadeDuration);
    }
    else
    {
      // Crossfade: first fade out current track
      StartFade(0.0f, fadeDuration * 0.5f); // Fade out in half the time
    }

    std::cout << "[BackgroundMusicSystem] Transitioning to stage: "
              << static_cast<int>(stage) << std::endl;
  }

  void BackgroundMusicSystem::FadeOut(float fadeDuration)
  {
    if (currentStage_ == MusicStage::None || currentTrackHandle_ == nullptr)
    {
      return;
    }

    isFadingOut_ = true;
    targetStage_ = MusicStage::None;
    fadeDuration_ = fadeDuration;
    StartFade(0.0f, fadeDuration);

    std::cout << "[BackgroundMusicSystem] Fading out over " << fadeDuration << " seconds" << std::endl;
  }

  void BackgroundMusicSystem::Stop()
  {
    if (currentTrackHandle_)
    {
      soundController_->StopSound(currentTrackHandle_);
      currentTrackHandle_ = nullptr;
    }

    currentStage_ = MusicStage::None;
    targetStage_ = MusicStage::None;
    isFading_ = false;
    isFadingOut_ = false;
    currentVolume_ = 0.0f;

    std::cout << "[BackgroundMusicSystem] Stopped" << std::endl;
  }

  void BackgroundMusicSystem::SetVolume(float volume)
  {
    baseVolume_ = std::clamp(volume, 0.0f, 1.0f);

    // If not fading, update current volume immediately
    if (!isFading_ && currentTrackHandle_)
    {
      currentVolume_ = baseVolume_;
      soundController_->SetVolume(currentTrackHandle_, currentVolume_);
    }
  }

  void BackgroundMusicSystem::PlayNextTrack()
  {
    if (!soundController_ || currentStage_ == MusicStage::None)
    {
      return;
    }

    const auto &tracks = GetTracksForStage(currentStage_);
    if (tracks.empty())
    {
      return;
    }

    // Stop current track if any
    if (currentTrackHandle_)
    {
      soundController_->StopSound(currentTrackHandle_);
      currentTrackHandle_ = nullptr;
    }

    // Get the track to play
    const std::string &trackPath = tracks[currentTrackIndex_ % tracks.size()];

    // Play as 2D sound (background music), not looped (we handle looping manually)
    currentTrackHandle_ = soundController_->Play2D(trackPath.c_str(), false, currentVolume_);

    if (currentTrackHandle_)
    {
      std::cout << "[BackgroundMusicSystem] Playing track: " << trackPath << std::endl;
    }
    else
    {
      std::cerr << "[BackgroundMusicSystem] Failed to play track: " << trackPath << std::endl;
    }
  }

  const std::vector<std::string> &BackgroundMusicSystem::GetTracksForStage(MusicStage stage) const
  {
    static const std::vector<std::string> empty;

    switch (stage)
    {
    case MusicStage::Normal:
      return normalTracks_;
    case MusicStage::BossFight:
      return bossFightTracks_;
    default:
      return empty;
    }
  }

  void BackgroundMusicSystem::StartFade(float targetVolume, float duration)
  {
    isFading_ = true;
    fadeTimer_ = 0.0f;
    fadeDuration_ = std::max(0.1f, duration);
    fadeStartVolume_ = currentVolume_;
    fadeTargetVolume_ = targetVolume;
  }

  void BackgroundMusicSystem::CheckTrackCompletion()
  {
    if (!soundController_ || currentStage_ == MusicStage::None || isFading_)
    {
      return;
    }

    // Check if current track has finished
    if (currentTrackHandle_ && !soundController_->IsPlaying(currentTrackHandle_))
    {
      // Track finished, play next
      const auto &tracks = GetTracksForStage(currentStage_);
      currentTrackIndex_ = (currentTrackIndex_ + 1) % tracks.size();

      std::cout << "[BackgroundMusicSystem] Track finished, playing next (index: "
                << currentTrackIndex_ << ")" << std::endl;

      currentTrackHandle_ = nullptr;
      PlayNextTrack();

      // Set volume immediately (no fade between tracks in same stage)
      if (currentTrackHandle_)
      {
        soundController_->SetVolume(currentTrackHandle_, baseVolume_);
        currentVolume_ = baseVolume_;
      }
    }
  }

} // namespace mecha
