#pragma once

namespace mecha
{
  // Reserve a high-numbered texture unit for the shadow depth map to avoid
  // conflicting with material textures bound by Model::Draw (which uses units starting at 0).
  inline constexpr int kShadowMapTextureUnit = 15;
  inline constexpr int kSSAOTexUnit = 14;

  // Default resolution for directional shadow maps. High resolution reduces pixelation at close range.
  inline constexpr unsigned int kShadowMapResolution = 16384;
  inline constexpr unsigned int kSSAOKernelSize = 64;
  inline constexpr unsigned int kSSAONoiseDimension = 4;
} // namespace mecha

