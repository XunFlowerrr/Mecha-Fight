#include "ParticleSystemBase.h"

#include <learnopengl/shader_m.h>
#include <glad/glad.h>

namespace mecha
{

  void ParticleSystemBase::Render(const RenderContext &ctx)
  {
    if (ctx.shadowPass)
    {
      return;
    }

    if (particles_.empty() || !shader_ || sphereVAO_ == 0 || sphereIndexCount_ == 0)
    {
      return;
    }

    // Enable blending for particle transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth writing so particles don't block each other
    glDepthMask(GL_FALSE);

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);

    glBindVertexArray(sphereVAO_);

    for (const auto &p : particles_)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, p.position);
      model = glm::scale(model, glm::vec3(ParticleRadius(p)));
      shader_->setMat4("model", model);
      shader_->setVec4("color", ParticleColor(p, renderParams_));
      glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
  }

  void ParticleSystemBase::SetRenderParams(const RenderParams &params)
  {
    renderParams_ = params;
  }

  void ParticleSystemBase::SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount)
  {
    shader_ = shader;
    sphereVAO_ = sphereVAO;
    sphereIndexCount_ = sphereIndexCount;
  }

} // namespace mecha
