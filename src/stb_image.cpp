#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef STB_IMAGE_NEEDS_16BIT_FROM_MEMORY_STUB
extern "C"
{

  int stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len)
  {
    (void)buffer;
    (void)len;
    return 0;
  }

  stbi_us *stbi_load_16_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels)
  {
    (void)buffer;
    (void)len;
    (void)x;
    (void)y;
    (void)channels_in_file;
    (void)desired_channels;
    return nullptr;
  }

} // extern "C"
#endif
