#include "SkyboxLoader.h"

#include <array>
#include <cstring>
#include <glad/glad.h>
#include <iostream>
#include <stb_image.h>
#include <vector>

namespace mecha
{
  namespace
  {
    struct FaceRegion
    {
      int col;
      int row;
    };

    constexpr std::array<FaceRegion, 6> kVerticalCrossLayout = {
        FaceRegion{2, 1}, // +X
        FaceRegion{0, 1}, // -X
        FaceRegion{1, 0}, // +Y
        FaceRegion{1, 2}, // -Y
        FaceRegion{1, 1}, // +Z
        FaceRegion{3, 1}  // -Z
    };
  } // namespace

  namespace SkyboxLoader
  {
    unsigned int LoadVerticalCrossCubemap(const std::string &imagePath)
    {
      int width = 0;
      int height = 0;
      int channels = 0;
      stbi_uc *pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
      if (!pixels)
      {
        std::cerr << "[SkyboxLoader] Failed to load '" << imagePath << "' (" << stbi_failure_reason() << ")" << std::endl;
        return 0;
      }

      const int faceSize = width / 4;
      if (faceSize <= 0 || width != faceSize * 4 || height != faceSize * 3)
      {
        std::cerr << "[SkyboxLoader] Unexpected cubemap layout. Expected 4x3 tiles, got " << width << "x" << height << std::endl;
        stbi_image_free(pixels);
        return 0;
      }

      std::vector<unsigned char> facePixels(faceSize * faceSize * 4);

      unsigned int cubemapTex = 0;
      glGenTextures(1, &cubemapTex);
      glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);

      GLint prevAlignment = 0;
      glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlignment);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      for (size_t faceIndex = 0; faceIndex < kVerticalCrossLayout.size(); ++faceIndex)
      {
        const FaceRegion &region = kVerticalCrossLayout[faceIndex];
        const int srcX = region.col * faceSize;
        const int srcY = region.row * faceSize;

        for (int y = 0; y < faceSize; ++y)
        {
          const int srcRow = srcY + y;
          const unsigned char *srcPtr = pixels + ((srcRow * width + srcX) * 4);
          unsigned char *dstPtr = facePixels.data() + (y * faceSize * 4);
          std::memcpy(dstPtr, srcPtr, faceSize * 4);
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(faceIndex),
                     0,
                     GL_RGBA,
                     faceSize,
                     faceSize,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     facePixels.data());
      }

      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlignment);
      glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
      stbi_image_free(pixels);

      std::cout << "[SkyboxLoader] Loaded cubemap '" << imagePath << "' (face size " << faceSize << ")" << std::endl;
      return cubemapTex;
    }
  } // namespace SkyboxLoader
} // namespace mecha


