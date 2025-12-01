#pragma once

#include <string>

namespace mecha
{
  namespace SkyboxLoader
  {
    /**
     * @brief Load a cubemap texture from a vertical-cross PNG (4x3 faces)
     * @param imagePath Path to the combined cubemap texture
     * @return GL texture ID for the cubemap, or 0 on failure
     */
    unsigned int LoadVerticalCrossCubemap(const std::string &imagePath);
  } // namespace SkyboxLoader
} // namespace mecha


