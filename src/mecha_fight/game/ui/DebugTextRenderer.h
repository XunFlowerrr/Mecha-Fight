#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <string>

class Shader;

namespace mecha
{

  class DebugTextRenderer
  {
  public:
    bool Init(unsigned int width, unsigned int height, const std::string &fontPath, unsigned int fontSize);
    void RenderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color);
    void Resize(unsigned int width, unsigned int height);
    bool IsReady() const { return initialized_; }

  private:
    struct DebugGlyph
    {
      unsigned int textureID = 0;
      glm::ivec2 size = glm::ivec2(0);
      glm::ivec2 bearing = glm::ivec2(0);
      unsigned int advance = 0;
    };

    bool LoadFont(const std::string &fontPath, unsigned int fontSize);

    std::map<char, DebugGlyph> glyphs_;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    std::unique_ptr<Shader> shader_;
    glm::mat4 projection_ = glm::mat4(1.0f);
    bool initialized_ = false;
  };

} // namespace mecha
