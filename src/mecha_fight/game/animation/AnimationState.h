#pragma once

namespace mecha
{
  struct AnimationState
  {
    bool paused{false};
    float speed{1.0f};

    float AdvanceAmount(float deltaTime) const
    {
      return paused ? 0.0f : deltaTime * speed;
    }
  };

} // namespace mecha
