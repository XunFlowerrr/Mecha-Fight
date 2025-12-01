#include "ProximitySoundSystem.h"
#include <algorithm>
#include <cmath>

namespace mecha
{

  ProximitySoundSystem::ProximitySoundSystem(ISoundController* soundController)
    : soundController_(soundController)
  {
    if (!soundController_)
    {
      // Log warning but don't crash - system will just not work
    }
  }

  void* ProximitySoundSystem::RegisterSound(const char* filePath, const glm::vec3& position, bool looped, 
                                            float baseVolume, float maxDistance)
  {
    if (!soundController_)
    {
      return nullptr;
    }

    // Play the sound through the sound controller
    void* handle = soundController_->Play3D(filePath, position, looped, baseVolume);
    
    if (handle)
    {
      // Track it in our system
      SoundInfo info;
      info.soundHandle = handle;
      info.position = position;
      info.maxDistance = maxDistance;
      info.baseVolume = baseVolume;
      info.filePath = filePath;
      info.isLooped = looped;
      
      activeSounds_.push_back(info);
    }

    return handle;
  }

  void ProximitySoundSystem::UpdateSoundPosition(void* soundHandle, const glm::vec3& newPosition)
  {
    SoundInfo* info = FindSoundInfo(soundHandle);
    if (info && soundController_)
    {
      info->position = newPosition;
      soundController_->SetPosition(soundHandle, newPosition);
    }
  }

  void ProximitySoundSystem::UnregisterSound(void* soundHandle)
  {
    if (!soundController_)
    {
      return;
    }

    // Stop the sound
    soundController_->StopSound(soundHandle);

    // Remove from tracking
    activeSounds_.erase(
      std::remove_if(activeSounds_.begin(), activeSounds_.end(),
        [soundHandle](const SoundInfo& info) {
          return info.soundHandle == soundHandle;
        }),
      activeSounds_.end()
    );
  }

  void ProximitySoundSystem::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
  {
    listenerPosition_ = position;
    listenerForward_ = glm::normalize(forward);
    listenerUp_ = glm::normalize(up);

    // Update the sound controller's listener position
    if (soundController_)
    {
      soundController_->SetListenerPosition(position, forward, up);
    }
  }

  float ProximitySoundSystem::CalculateVolume(float distance, float maxDistance, float baseVolume)
  {
    if (maxDistance <= 0.0f)
    {
      return baseVolume; // No attenuation if maxDistance is invalid
    }

    if (distance >= maxDistance)
    {
      return 0.0f; // Beyond max distance, volume is 0
    }

    // Use a quadratic (ease-out) falloff so sounds retain volume longer near the listener
    float normalized = std::clamp(distance / maxDistance, 0.0f, 1.0f);
    float attenuation = 1.0f - normalized * normalized;
    return baseVolume * std::max(0.0f, attenuation);
  }

  ProximitySoundSystem::SoundInfo* ProximitySoundSystem::FindSoundInfo(void* soundHandle)
  {
    auto it = std::find_if(activeSounds_.begin(), activeSounds_.end(),
      [soundHandle](const SoundInfo& info) {
        return info.soundHandle == soundHandle;
      });

    if (it != activeSounds_.end())
    {
      return &(*it);
    }
    return nullptr;
  }

  void ProximitySoundSystem::CleanupFinishedSounds()
  {
    if (!soundController_)
    {
      return;
    }

    // Remove sounds that are no longer playing
    activeSounds_.erase(
      std::remove_if(activeSounds_.begin(), activeSounds_.end(),
        [this](const SoundInfo& info) {
          // Keep looped sounds, remove finished non-looped sounds
          if (info.isLooped)
          {
            return false; // Keep looped sounds
          }
          return !soundController_->IsPlaying(info.soundHandle);
        }),
      activeSounds_.end()
    );
  }

  void ProximitySoundSystem::Update(const UpdateContext& ctx)
  {
    if (!soundController_)
    {
      return;
    }

    // Clean up finished sounds first
    CleanupFinishedSounds();

    // Update volume for each active sound based on distance
    for (auto& soundInfo : activeSounds_)
    {
      // Calculate distance from listener to sound source
      float distance = glm::length(soundInfo.position - listenerPosition_);

      // Calculate volume using linear attenuation
      float volume = CalculateVolume(distance, soundInfo.maxDistance, soundInfo.baseVolume);

      // Update the sound volume
      soundController_->SetVolume(soundInfo.soundHandle, volume);
    }
  }

} // namespace mecha

