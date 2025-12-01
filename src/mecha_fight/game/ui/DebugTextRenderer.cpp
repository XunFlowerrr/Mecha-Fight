#include "DebugTextRenderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>

#include <iostream>

namespace mecha
{

  bool DebugTextRenderer::Init(unsigned int width, unsigned int height, const std::string &fontPath, unsigned int fontSize)
  {
    const std::string vertexPath = FileSystem::getPath("src/mecha_fight/shaders/dev_text.vs");
    const std::string fragmentPath = FileSystem::getPath("src/mecha_fight/shaders/dev_text.fs");
    shader_ = std::make_unique<Shader>(vertexPath.c_str(), fragmentPath.c_str());

    projection_ = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
    shader_->use();
    shader_->setMat4("projection", projection_);
    shader_->setInt("text", 0);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized_ = LoadFont(fontPath, fontSize);
    return initialized_;
  }

  bool DebugTextRenderer::LoadFont(const std::string &fontPath, unsigned int fontSize)
  {
    glyphs_.clear();
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
      return false;
    }

    FT_Face face;
    if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
    {
      std::cout << "ERROR::FREETYPE: Failed to load font " << fontPath << std::endl;
      FT_Done_FreeType(ft);
      return false;
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; ++c)
    {
      if (FT_Load_Char(face, c, FT_LOAD_RENDER))
      {
        std::cout << "ERROR::FREETYPE: Failed to load Glyph for character " << static_cast<int>(c) << std::endl;
        continue;
      }

      unsigned int texture;
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                   face->glyph->bitmap.width,
                   face->glyph->bitmap.rows,
                   0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      DebugGlyph glyph;
      glyph.textureID = texture;
      glyph.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
      glyph.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
      glyph.advance = static_cast<unsigned int>(face->glyph->advance.x);
      glyphs_.insert(std::make_pair(static_cast<char>(c), glyph));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return true;
  }

  void DebugTextRenderer::Resize(unsigned int width, unsigned int height)
  {
    if (!initialized_ || !shader_)
    {
      return;
    }
    projection_ = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
    shader_->use();
    shader_->setMat4("projection", projection_);
  }

  void DebugTextRenderer::RenderText(const std::string &text, float x, float y, float scale, const glm::vec3 &color)
  {
    if (!initialized_ || !shader_)
    {
      return;
    }

    shader_->use();
    shader_->setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao_);

    for (char c : text)
    {
      const auto it = glyphs_.find(c);
      if (it == glyphs_.end())
      {
        continue;
      }

      const DebugGlyph &glyph = it->second;
      float xpos = x + glyph.bearing.x * scale;
      float ypos = y - (glyph.size.y - glyph.bearing.y) * scale;
      float w = glyph.size.x * scale;
      float h = glyph.size.y * scale;

      float vertices[6][4] = {
          {xpos, ypos + h, 0.0f, 1.0f},
          {xpos + w, ypos, 1.0f, 0.0f},
          {xpos, ypos, 0.0f, 0.0f},

          {xpos, ypos + h, 0.0f, 1.0f},
          {xpos + w, ypos + h, 1.0f, 1.0f},
          {xpos + w, ypos, 1.0f, 0.0f}};

      glBindTexture(GL_TEXTURE_2D, glyph.textureID);
      glBindBuffer(GL_ARRAY_BUFFER, vbo_);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glDrawArrays(GL_TRIANGLES, 0, 6);

      x += (glyph.advance >> 6) * scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

} // namespace mecha
