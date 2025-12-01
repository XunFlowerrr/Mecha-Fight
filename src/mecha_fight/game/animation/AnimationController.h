#pragma once

#include <unordered_map>
#include <algorithm>
#include <utility>

class Model;

#include "AnimationState.h"

namespace mecha
{
  class AnimationController
  {
  public:
    enum class PlaybackMode
    {
      StaticPose,
      LoopingAnimation
    };

    struct ActionConfig
    {
      int clipIndex{-1};
      PlaybackMode mode{PlaybackMode::StaticPose};
      bool usePlaybackWindow{false};
      float playbackStartNormalized{0.0f};
      float playbackEndNormalized{1.0f};
      float transitionDuration{0.15f};
    };

    void BindModel(Model *model);
    void RegisterAction(int actionId, const ActionConfig &config);
    void ClearActions();

    void SetAction(int actionId);
    int GetCurrentAction() const { return currentAction_; }

    void SetControls(bool paused, float speed);
    void Update(float deltaTime);

  private:
    void applyConfig(const ActionConfig &config);
    void applyPlaybackWindow(const ActionConfig &config);
    void startTransition(const ActionConfig &config);
    std::pair<float, float> computePlaybackWindowSeconds(const ActionConfig &config) const;

    Model *model_{nullptr};
    AnimationState state_{};
    std::unordered_map<int, ActionConfig> actionConfigs_{};
    int currentAction_{-1};
    bool playing_{false};
  };

} // namespace mecha
