#include "AnimationController.h"

#include <learnopengl/model.h>

namespace mecha
{
  void AnimationController::BindModel(Model *model)
  {
    model_ = model;
    if (model_ && currentAction_ >= 0)
    {
      auto it = actionConfigs_.find(currentAction_);
      if (it != actionConfigs_.end())
      {
        applyConfig(it->second);
      }
    }
  }

  void AnimationController::RegisterAction(int actionId, const ActionConfig &config)
  {
    actionConfigs_[actionId] = config;
    if (actionId == currentAction_ && model_)
    {
      applyConfig(config);
    }
  }

  void AnimationController::ClearActions()
  {
    actionConfigs_.clear();
    currentAction_ = -1;
    playing_ = false;
  }

  void AnimationController::SetAction(int actionId)
  {
    if (currentAction_ == actionId)
    {
      return;
    }
    auto it = actionConfigs_.find(actionId);
    if (it == actionConfigs_.end())
    {
      currentAction_ = actionId;
      playing_ = false;
      return;
    }

    auto prevIt = actionConfigs_.find(currentAction_);
    const bool canBlend = model_ && model_->HasAnimations() && currentAction_ >= 0 &&
                          prevIt != actionConfigs_.end() && prevIt->second.clipIndex >= 0 &&
                          it->second.clipIndex >= 0 &&
                          it->second.transitionDuration > 0.0f;

    if (canBlend)
    {
      startTransition(it->second);
    }
    else
    {
      applyConfig(it->second);
    }

    currentAction_ = actionId;
  }

  void AnimationController::SetControls(bool paused, float speed)
  {
    state_.paused = paused;
    state_.speed = speed;
  }

  void AnimationController::Update(float deltaTime)
  {
    if (!model_ || !model_->HasAnimations())
    {
      return;
    }

    bool shouldUpdate = playing_ || model_->IsAnimationBlendActive();
    if (!shouldUpdate)
    {
      return;
    }

    float delta = state_.AdvanceAmount(deltaTime);
    if (delta != 0.0f)
    {
      model_->UpdateAnimation(delta);
    }
  }

  void AnimationController::applyConfig(const ActionConfig &config)
  {
    if (!model_ || !model_->HasAnimations())
    {
      playing_ = false;
      return;
    }

    if (config.clipIndex >= 0)
    {
      model_->SetActiveAnimation(config.clipIndex);
      // Force an initial animation update to ensure skin matrices are calculated
      // This ensures the animation pose is applied immediately, not just on the next frame
      model_->UpdateAnimation(0.0f);
    }

    if (config.usePlaybackWindow)
    {
      applyPlaybackWindow(config);
    }
    else if (model_)
    {
      model_->ClearAnimationPlaybackWindow();
    }

    playing_ = (config.mode == PlaybackMode::LoopingAnimation);
  }

  void AnimationController::applyPlaybackWindow(const ActionConfig &config)
  {
    if (!model_)
    {
      return;
    }

    auto window = computePlaybackWindowSeconds(config);
    if (window.second <= window.first)
    {
      model_->ClearAnimationPlaybackWindow();
      return;
    }

    model_->SetAnimationPlaybackWindow(window.first, window.second);
  }

  void AnimationController::startTransition(const ActionConfig &config)
  {
    if (!model_ || config.clipIndex < 0)
    {
      applyConfig(config);
      return;
    }

    auto window = computePlaybackWindowSeconds(config);
    if (window.second <= window.first)
    {
      applyConfig(config);
      return;
    }

    model_->StartAnimationBlend(config.clipIndex,
                                std::max(config.transitionDuration, 0.0f),
                                config.usePlaybackWindow,
                                window.first,
                                window.second);
    playing_ = (config.mode == PlaybackMode::LoopingAnimation);
  }

  std::pair<float, float> AnimationController::computePlaybackWindowSeconds(const ActionConfig &config) const
  {
    if (!model_)
    {
      return {0.0f, 0.0f};
    }

    int clipIndex = config.clipIndex >= 0 ? config.clipIndex : model_->GetActiveAnimationIndex();
    float duration = model_->GetAnimationClipDuration(clipIndex);
    if (duration <= 0.0f)
    {
      return {0.0f, 0.0f};
    }

    if (!config.usePlaybackWindow)
    {
      return {0.0f, duration};
    }

    float startNorm = std::clamp(config.playbackStartNormalized, 0.0f, 1.0f);
    float endNorm = std::clamp(config.playbackEndNormalized, startNorm + 0.001f, 1.0f);
    float start = duration * startNorm;
    float end = duration * endNorm;
    return {start, end};
  }

} // namespace mecha
