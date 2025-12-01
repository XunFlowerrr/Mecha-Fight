#pragma once

#include <glm/glm.hpp>

class Shader;

struct GLFWwindow;

namespace mecha
{
  struct Transform
  {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
  };

  struct UpdateContext
  {
    float deltaTime{0.0f};
    GLFWwindow *window{nullptr};
    void *userData{nullptr};
  };

  struct RenderContext
  {
    float deltaTime{0.0f};
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::vec3 viewPos{0.0f};
    glm::mat4 lightSpaceMatrix{1.0f};
    glm::vec3 lightPos{0.0f};
    glm::vec3 lightIntensity{1.0f, 1.0f, 1.0f};
    unsigned int shadowMapTexture{0};
    unsigned int ssaoTexture{0};
    glm::vec2 screenSize{0.0f};
    bool shadowPass{false};
    Shader *overrideShader{nullptr};
    bool ssaoEnabled{false};
    float ssaoStrength{1.0f};
  };

  class Entity
  {
  public:
    virtual ~Entity() = default;

    virtual void Update(const UpdateContext &)
    {
    }

    virtual void FixedUpdate(const UpdateContext &)
    {
    }

    virtual void Render(const RenderContext &)
    {
    }

    void SetFramePayload(void *payload)
    {
      framePayload_ = payload;
    }

    Transform &GetTransform()
    {
      return transform_;
    }

    const Transform &GetTransform() const
    {
      return transform_;
    }

  protected:
    Transform transform_{};
    void *GetFramePayload() const
    {
      return framePayload_;
    }

  private:
    void *framePayload_{nullptr};
  };

} // namespace mecha
