#pragma once

#include <glad/glad.h>

namespace mecha
{

  // Simple mesh handle for placeholder geometry (currently a UV sphere).
  struct MeshHandle
  {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int indexCount = 0;
  };

  // Creates a placeholder sphere used for enemy and projectile visuals until a model is integrated.
  // stacks/slices control tessellation; defaults chosen for moderate density.
  MeshHandle CreateEnemyPlaceholderSphere(int stacks = 16, int slices = 24);

} // namespace mecha
