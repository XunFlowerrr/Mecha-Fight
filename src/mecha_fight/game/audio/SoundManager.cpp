#include "SoundManager.h"
#include <iostream>

namespace mecha
{

  SoundManager::SoundManager(ISoundController *soundController)
      : soundController_(soundController)
  {
    if (!soundController_)
    {
      std::cerr << "[SoundManager] Warning: Sound controller is null!" << std::endl;
      return;
    }

    // Create proximity sound system
    proximitySystem_ = std::make_shared<ProximitySoundSystem>(soundController_);
  }

  void SoundManager::RegisterSound(const std::string &name, const SoundConfig &config)
  {
    if (config.filePath == nullptr)
    {
      std::cerr << "[SoundManager] Warning: Attempted to register sound '" << name
                << "' with null file path" << std::endl;
      return;
    }

    registeredSounds_[name] = config;
  }

  void *SoundManager::PlaySound3D(const std::string &name, const glm::vec3 &position,
                                  float volumeOverride, float maxDistanceOverride)
  {
    if (!soundController_ || !proximitySystem_)
    {
      return nullptr;
    }

    auto it = registeredSounds_.find(name);
    if (it == registeredSounds_.end())
    {
      std::cerr << "[SoundManager] Warning: Sound '" << name << "' not registered!" << std::endl;
      return nullptr;
    }

    const SoundConfig &config = it->second;
    if (config.minInterval > 0.0f)
    {
      auto lastIt = lastPlayTime_.find(name);
      if (lastIt != lastPlayTime_.end())
      {
        if ((elapsedTime_ - lastIt->second) < config.minInterval)
        {
          return nullptr;
        }
      }
    }

    float volume = (volumeOverride >= 0.0f) ? volumeOverride : config.defaultVolume;

    // Apply per-sound volume multiplier if set
    auto volumeIt = soundVolumes_.find(name);
    if (volumeIt != soundVolumes_.end())
    {
      volume *= volumeIt->second;
    }

    float maxDist = (maxDistanceOverride >= 0.0f) ? maxDistanceOverride : config.maxDistance;

    void *handle = proximitySystem_->RegisterSound(
        config.filePath,
        position,
        config.isLooped,
        volume,
        maxDist);

    if (handle && config.minInterval > 0.0f)
    {
      lastPlayTime_[name] = elapsedTime_;
    }

    return handle;
  }

  void *SoundManager::PlaySound2D(const std::string &name, float volumeOverride)
  {
    if (!soundController_)
    {
      return nullptr;
    }

    auto it = registeredSounds_.find(name);
    if (it == registeredSounds_.end())
    {
      std::cerr << "[SoundManager] Warning: Sound '" << name << "' not registered!" << std::endl;
      return nullptr;
    }

    const SoundConfig &config = it->second;
    if (config.minInterval > 0.0f)
    {
      auto lastIt = lastPlayTime_.find(name);
      if (lastIt != lastPlayTime_.end())
      {
        if ((elapsedTime_ - lastIt->second) < config.minInterval)
        {
          return nullptr;
        }
      }
    }

    float volume = (volumeOverride >= 0.0f) ? volumeOverride : config.defaultVolume;

    // Apply per-sound volume multiplier if set
    auto volumeIt = soundVolumes_.find(name);
    if (volumeIt != soundVolumes_.end())
    {
      volume *= volumeIt->second;
    }

    void *handle = soundController_->Play2D(config.filePath, config.isLooped, volume);
    if (handle && config.minInterval > 0.0f)
    {
      lastPlayTime_[name] = elapsedTime_;
    }
    return handle;
  }

  void SoundManager::UpdateSoundPosition(void *soundHandle, const glm::vec3 &newPosition)
  {
    if (proximitySystem_)
    {
      proximitySystem_->UpdateSoundPosition(soundHandle, newPosition);
    }
  }

  void SoundManager::SetSoundPitch(void *soundHandle, float pitch)
  {
    if (soundController_)
    {
      soundController_->SetPitch(soundHandle, pitch);
    }
  }

  void SoundManager::StopSound(void *soundHandle)
  {
    if (proximitySystem_)
    {
      proximitySystem_->UnregisterSound(soundHandle);
    }
    else if (soundController_)
    {
      soundController_->StopSound(soundHandle);
    }
  }

  void SoundManager::SetListenerPosition(const glm::vec3 &position, const glm::vec3 &forward, const glm::vec3 &up)
  {
    if (proximitySystem_)
    {
      proximitySystem_->SetListenerPosition(position, forward, up);
    }
  }

  void SoundManager::Update(float deltaTime)
  {
    elapsedTime_ += deltaTime;
    if (soundController_)
    {
      soundController_->Update(deltaTime);
    }
  }

  void SoundManager::SetMasterVolume(float volume)
  {
    if (soundController_)
    {
      soundController_->SetMasterVolume(volume);
    }
  }

  float SoundManager::GetMasterVolume() const
  {
    if (soundController_)
    {
      return soundController_->GetMasterVolume();
    }
    return 1.0f;
  }

  void SoundManager::PreloadSound(const std::string &name)
  {
    if (!soundController_)
    {
      return;
    }

    auto it = registeredSounds_.find(name);
    if (it == registeredSounds_.end())
    {
      return;
    }

    soundController_->PreloadSound(it->second.filePath);
  }

  void SoundManager::SetSoundVolume(const std::string &name, float volume)
  {
    volume = std::max(0.0f, std::min(1.0f, volume));
    soundVolumes_[name] = volume;
  }

  float SoundManager::GetSoundVolume(const std::string &name) const
  {
    auto it = soundVolumes_.find(name);
    if (it != soundVolumes_.end())
    {
      return it->second;
    }
    return 1.0f; // Default to full volume if not set
  }

} // namespace mecha
