#pragma once

#include "ISoundController.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Forward declarations for miniaudio
struct ma_engine;
struct ma_sound;
struct ma_decoder;

namespace mecha
{

  /**
   * @brief Concrete implementation of ISoundController using miniaudio
   * 
   * Cross-platform sound controller that works on Windows, macOS (including arm64),
   * Linux, and other platforms. Provides 3D positional audio support.
   */
  class MiniaudioSoundController : public ISoundController
  {
  public:
    MiniaudioSoundController();
    virtual ~MiniaudioSoundController();

    // ISoundController interface
    void* Play3D(const char* filePath, const glm::vec3& position, bool looped = false, float volume = 1.0f) override;
    void* Play2D(const char* filePath, bool looped = false, float volume = 1.0f) override;
    void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) override;
    void StopSound(void* soundHandle) override;
    void SetVolume(void* soundHandle, float volume) override;
    void SetPosition(void* soundHandle, const glm::vec3& position) override;
    void SetPitch(void* soundHandle, float pitch) override;
    bool IsPlaying(void* soundHandle) const override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    void PreloadSound(const char* filePath) override;
    void SetMasterVolume(float volume) override;
    float GetMasterVolume() const override;

  private:
    ma_engine* engine_;
    
    // Track active sounds
    struct SoundHandle
    {
      ma_sound* sound{nullptr};
      ma_decoder* decoder{nullptr}; // Owned decoder for memory-based sounds
      bool is3D{false};
      bool isLooped{false};
    };
    
    std::unordered_map<void*, std::unique_ptr<SoundHandle>> activeSounds_;
    void* nextHandleId_{nullptr};
    std::unordered_map<std::string, std::shared_ptr<std::vector<unsigned char>>> cachedAudio_;
    
    // Helper to get SoundHandle from void*
    SoundHandle* GetSoundHandle(void* handle) const;
    
    // Helper to create new handle ID
    void* CreateHandle();
    
    // Clean up finished sounds
    void CleanupFinishedSounds();

    std::shared_ptr<std::vector<unsigned char>> GetOrLoadAudioData(const char* filePath);
  };

} // namespace mecha

