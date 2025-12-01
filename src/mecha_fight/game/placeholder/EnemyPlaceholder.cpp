#include "EnemyPlaceholder.h"

#include <vector>
#include <cmath>

namespace mecha
{

  MeshHandle CreateEnemyPlaceholderSphere(int stacks, int slices)
  {
    MeshHandle handle{};

    std::vector<float> verts; // positions only (x,y,z)
    std::vector<unsigned int> inds;

    verts.reserve((stacks + 1) * (slices + 1) * 3);
    inds.reserve(stacks * slices * 6);

    const float PI = 3.1415926f;
    for (int i = 0; i <= stacks; ++i)
    {
      float v = i / static_cast<float>(stacks);
      float phi = v * PI;
      for (int j = 0; j <= slices; ++j)
      {
        float u = j / static_cast<float>(slices);
        float theta = u * 2.0f * PI;
        float x = std::sin(phi) * std::cos(theta);
        float y = std::cos(phi);
        float z = std::sin(phi) * std::sin(theta);
        verts.push_back(x);
        verts.push_back(y);
        verts.push_back(z);
      }
    }

    for (int i = 0; i < stacks; ++i)
    {
      for (int j = 0; j < slices; ++j)
      {
        int a = i * (slices + 1) + j;
        int b = a + slices + 1;
        inds.push_back(a);
        inds.push_back(b);
        inds.push_back(a + 1);
        inds.push_back(a + 1);
        inds.push_back(b);
        inds.push_back(b + 1);
      }
    }

    glGenVertexArrays(1, &handle.vao);
    glGenBuffers(1, &handle.vbo);
    glGenBuffers(1, &handle.ebo);

    glBindVertexArray(handle.vao);
    glBindBuffer(GL_ARRAY_BUFFER, handle.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(unsigned int), inds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    handle.indexCount = static_cast<int>(inds.size());
    return handle;
  }

} // namespace mecha
