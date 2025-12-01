#include "MiniaudioSoundController.h"

// Windows-specific defines to prevent conflicts
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

// Include miniaudio - single header library
// Download from: https://github.com/mackron/miniaudio
// Place miniaudio.h in includes/ directory
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <iostream>
#include <algorithm>
#include <fstream>

namespace mecha
{

  MiniaudioSoundController::MiniaudioSoundController()
    : engine_(nullptr)
    , nextHandleId_(reinterpret_cast<void*>(1))
  {
    // Allocate engine
    engine_ = new ma_engine;

    // Initialize engine
    ma_result result = ma_engine_init(nullptr, engine_);
    if (result != MA_SUCCESS)
    {
      std::cerr << "[Sound] Failed to initialize miniaudio engine: " << result << std::endl;
      delete engine_;
      engine_ = nullptr;
      return;
    }

    std::cout << "[Sound] miniaudio sound engine initialized successfully" << std::endl;
  }

  MiniaudioSoundController::~MiniaudioSoundController()
  {
    Shutdown();
  }

  void* MiniaudioSoundController::CreateHandle()
  {
    void* handle = nextHandleId_;
    nextHandleId_ = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(nextHandleId_) + 1);
    return handle;
  }

  MiniaudioSoundController::SoundHandle* MiniaudioSoundController::GetSoundHandle(void* handle) const
  {
    if (!handle)
    {
      return nullptr;
    }

    auto it = activeSounds_.find(handle);
    if (it != activeSounds_.end())
    {
      return it->second.get();
    }
    return nullptr;
  }

  std::shared_ptr<std::vector<unsigned char>> MiniaudioSoundController::GetOrLoadAudioData(const char* filePath)
  {
    if (!filePath)
    {
      return nullptr;
    }

    std::string key(filePath);
    auto it = cachedAudio_.find(key);
    if (it != cachedAudio_.end())
    {
      return it->second;
    }

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
      std::cerr << "[Sound] Failed to open audio file for preload: " << filePath << std::endl;
      return nullptr;
    }

    std::streamsize size = file.tellg();
    if (size <= 0)
    {
      std::cerr << "[Sound] Audio file empty: " << filePath << std::endl;
      return nullptr;
    }
    file.seekg(0, std::ios::beg);

    auto buffer = std::make_shared<std::vector<unsigned char>>(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer->data()), size))
    {
      std::cerr << "[Sound] Failed to read audio file: " << filePath << std::endl;
      return nullptr;
    }

    cachedAudio_[key] = buffer;
    return buffer;
  }

  void* MiniaudioSoundController::Play3D(const char* filePath, const glm::vec3& position, bool looped, float volume)
  {
    if (!engine_)
    {
      return nullptr;
    }

    // Create sound handle
    auto soundHandle = std::make_unique<SoundHandle>();
    soundHandle->sound = new ma_sound;
    soundHandle->is3D = true;
    soundHandle->isLooped = looped;

    // Load and configure 3D sound
    // For 3D sounds, we need spatialization enabled
    ma_result result = MA_ERROR;
    auto cached = GetOrLoadAudioData(filePath);
    if (cached)
    {
      // Create decoder from memory
      soundHandle->decoder = new ma_decoder;
      ma_decoder_config decoderConfig = ma_decoder_config_init_default();
      result = ma_decoder_init_memory(cached->data(), cached->size(), &decoderConfig, soundHandle->decoder);
      if (result == MA_SUCCESS)
      {
        // Create sound from decoder
        ma_sound_config soundConfig = ma_sound_config_init_2(engine_);
        soundConfig.pDataSource = soundHandle->decoder;
        soundConfig.flags = MA_SOUND_FLAG_DECODE;
        result = ma_sound_init_ex(engine_, &soundConfig, soundHandle->sound);
      }
      else
      {
        delete soundHandle->decoder;
        soundHandle->decoder = nullptr;
      }
    }
    else
    {
      result = ma_sound_init_from_file(engine_, filePath,
                                        MA_SOUND_FLAG_DECODE,
                                        nullptr,  // pGroup (null = default group)
                                        nullptr,  // pDoneFence (null = synchronous)
                                        soundHandle->sound);
    }

    if (result != MA_SUCCESS)
    {
      std::cerr << "[Sound] Failed to load 3D sound: " << filePath << " (error: " << result << ")" << std::endl;
      delete soundHandle->sound;
      return nullptr;
    }

    // Configure sound - disable spatialization since we handle all volume/positioning manually
    ma_sound_set_position(soundHandle->sound, position.x, position.y, position.z);
    ma_sound_set_spatialization_enabled(soundHandle->sound, MA_FALSE); // Disabled - we manage volume manually
    ma_sound_set_volume(soundHandle->sound, volume);

    if (looped)
    {
      ma_sound_set_looping(soundHandle->sound, MA_TRUE);
    }

    // Start playing
    ma_sound_start(soundHandle->sound);

    // Store handle
    void* handle = CreateHandle();
    activeSounds_[handle] = std::move(soundHandle);

    return handle;
  }

  void* MiniaudioSoundController::Play2D(const char* filePath, bool looped, float volume)
  {
    if (!engine_)
    {
      return nullptr;
    }

    // Create sound handle
    auto soundHandle = std::make_unique<SoundHandle>();
    soundHandle->sound = new ma_sound;
    soundHandle->is3D = false;
    soundHandle->isLooped = looped;

    // Load 2D sound (no spatialization)
    ma_result result = MA_ERROR;
    auto cached = GetOrLoadAudioData(filePath);
    if (cached)
    {
      // Create decoder from memory
      soundHandle->decoder = new ma_decoder;
      ma_decoder_config decoderConfig = ma_decoder_config_init_default();
      result = ma_decoder_init_memory(cached->data(), cached->size(), &decoderConfig, soundHandle->decoder);
      if (result == MA_SUCCESS)
      {
        // Create sound from decoder
        ma_sound_config soundConfig = ma_sound_config_init_2(engine_);
        soundConfig.pDataSource = soundHandle->decoder;
        soundConfig.flags = MA_SOUND_FLAG_DECODE;
        result = ma_sound_init_ex(engine_, &soundConfig, soundHandle->sound);
      }
      else
      {
        delete soundHandle->decoder;
        soundHandle->decoder = nullptr;
      }
    }
    else
    {
      result = ma_sound_init_from_file(engine_, filePath,
                                        MA_SOUND_FLAG_DECODE,
                                        nullptr,  // pGroup (null = default group)
                                        nullptr,  // pDoneFence (null = synchronous)
                                        soundHandle->sound);
    }

    if (result != MA_SUCCESS)
    {
      std::cerr << "[Sound] Failed to load 2D sound: " << filePath << " (error: " << result << ")" << std::endl;
      delete soundHandle->sound;
      return nullptr;
    }

    // Configure 2D sound
    ma_sound_set_spatialization_enabled(soundHandle->sound, MA_FALSE);
    ma_sound_set_volume(soundHandle->sound, volume);

    if (looped)
    {
      ma_sound_set_looping(soundHandle->sound, MA_TRUE);
    }

    // Start playing
    ma_sound_start(soundHandle->sound);

    // Store handle
    void* handle = CreateHandle();
    activeSounds_[handle] = std::move(soundHandle);

    return handle;
  }

  void MiniaudioSoundController::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up)
  {
    if (!engine_)
    {
      return;
    }

    // Set listener position and orientation
    ma_engine_listener_set_position(engine_, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(engine_, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(engine_, 0, up.x, up.y, up.z);
  }

  void MiniaudioSoundController::StopSound(void* soundHandle)
  {
    SoundHandle* handle = GetSoundHandle(soundHandle);
    if (handle && handle->sound)
    {
      ma_sound_stop(handle->sound);
      ma_sound_uninit(handle->sound);
      delete handle->sound;
      // Clean up decoder if it exists
      if (handle->decoder)
      {
        ma_decoder_uninit(handle->decoder);
        delete handle->decoder;
      }
      activeSounds_.erase(soundHandle);
    }
  }

  void MiniaudioSoundController::SetVolume(void* soundHandle, float volume)
  {
    SoundHandle* handle = GetSoundHandle(soundHandle);
    if (handle && handle->sound)
    {
      volume = std::max(0.0f, std::min(1.0f, volume));
      ma_sound_set_volume(handle->sound, volume);
    }
  }

  void MiniaudioSoundController::SetPosition(void* soundHandle, const glm::vec3& position)
  {
    SoundHandle* handle = GetSoundHandle(soundHandle);
    if (handle && handle->sound && handle->is3D)
    {
      ma_sound_set_position(handle->sound, position.x, position.y, position.z);
    }
  }

  void MiniaudioSoundController::SetPitch(void* soundHandle, float pitch)
  {
    SoundHandle* handle = GetSoundHandle(soundHandle);
    if (handle && handle->sound)
    {
      ma_sound_set_pitch(handle->sound, pitch);
    }
  }

  bool MiniaudioSoundController::IsPlaying(void* soundHandle) const
  {
    SoundHandle* handle = GetSoundHandle(soundHandle);
    if (handle && handle->sound)
    {
      return ma_sound_is_playing(handle->sound) != 0;
    }
    return false;
  }

  void MiniaudioSoundController::Update(float deltaTime)
  {
    if (!engine_)
    {
      return;
    }

    // Clean up finished sounds
    CleanupFinishedSounds();
  }

  void MiniaudioSoundController::CleanupFinishedSounds()
  {
    // Remove sounds that are no longer playing (except looped sounds)
    auto it = activeSounds_.begin();
    while (it != activeSounds_.end())
    {
      SoundHandle* handle = it->second.get();
      if (handle && handle->sound)
      {
        // Keep looped sounds, remove finished non-looped sounds
        if (!handle->isLooped && !ma_sound_is_playing(handle->sound))
        {
          ma_sound_uninit(handle->sound);
          delete handle->sound;
          // Clean up decoder if it exists
          if (handle->decoder)
          {
            ma_decoder_uninit(handle->decoder);
            delete handle->decoder;
          }
          it = activeSounds_.erase(it);
        }
        else
        {
          ++it;
        }
      }
      else
      {
        ++it;
      }
    }
  }

  void MiniaudioSoundController::Shutdown()
  {
    // Stop and cleanup all sounds
    for (auto& pair : activeSounds_)
    {
      SoundHandle* handle = pair.second.get();
      if (handle && handle->sound)
      {
        ma_sound_stop(handle->sound);
        ma_sound_uninit(handle->sound);
        delete handle->sound;
      }
      // Clean up decoder if it exists
      if (handle && handle->decoder)
      {
        ma_decoder_uninit(handle->decoder);
        delete handle->decoder;
      }
    }
    activeSounds_.clear();

    if (engine_)
    {
      ma_engine_uninit(engine_);
      delete engine_;
      engine_ = nullptr;
      std::cout << "[Sound] miniaudio sound engine shut down" << std::endl;
    }
  }

  void MiniaudioSoundController::SetMasterVolume(float volume)
  {
    if (engine_)
    {
      constexpr float kMaxVolume = 2.0f;
      volume = std::max(0.0f, std::min(kMaxVolume, volume));
      ma_engine_set_volume(engine_, volume);
    }
  }

  float MiniaudioSoundController::GetMasterVolume() const
  {
    if (engine_)
    {
      return ma_engine_get_volume(engine_);
    }
    return 1.0f;
  }

  void MiniaudioSoundController::PreloadSound(const char* filePath)
  {
    if (!engine_ || !filePath)
    {
      return;
    }
    GetOrLoadAudioData(filePath);
  }

} // namespace mecha

