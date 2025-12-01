#include "PortalGate.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "../rendering/RenderConstants.h"
#include "../GameplayTypes.h"
#include "../audio/SoundManager.h"
#include "../audio/SoundRegistry.h"
#include <learnopengl/model.h>
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kRadius = 2.0f;
    constexpr float kMaxHP = 500.0f;
    constexpr float kHeightOffset = 3.0f; // Height offset above terrain
  }

  PortalGate::PortalGate()
  {
    transform_.position = glm::vec3(0.0f, 0.0f, 0.0f);
    hp_ = kMaxHP;
    alive_ = true;
  }

  bool PortalGate::IsAlive() const
  {
    return alive_;
  }

  float PortalGate::Radius() const
  {
    return kRadius;
  }

  const glm::vec3 &PortalGate::Position() const
  {
    return transform_.position;
  }

  float PortalGate::HitPoints() const
  {
    return hp_;
  }

  void PortalGate::ApplyDamage(float amount)
  {
    if (!alive_)
    {
      return;
    }

    hp_ -= amount;
    
    // Spawn spark particles at gate position when hit
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (params && params->sparkParticles)
    {
      SpawnSparkParticles(transform_.position, params);
    }
    
    if (hp_ <= 0.0f)
    {
      alive_ = false;
      
      // Play gate collapsing sound
      if (params && params->soundManager)
      {
        params->soundManager->PlaySound3D("GATE_COLLAPSE", transform_.position);
      }
      
      std::cout << "[PortalGate] Gate destroyed at position (" 
                << transform_.position.x << ", " << transform_.position.y << ", " << transform_.position.z << ")" << std::endl;
    }
  }

  void PortalGate::SpawnSparkParticles(const glm::vec3 &hitPosition, const UpdateParams *params) const
  {
    if (!params || !params->sparkParticles)
    {
      return;
    }

    constexpr int kSparkCount = 20;
    constexpr float kSparkSpeed = 8.0f;
    constexpr float kSparkLife = 0.5f;

    auto randFloat = []() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); };
    auto randSigned = [&]() { return randFloat() * 2.0f - 1.0f; };

    auto &particles = *params->sparkParticles;
    for (int i = 0; i < kSparkCount; ++i)
    {
      SparkParticle spark;
      spark.pos = hitPosition + glm::vec3(randSigned() * 0.5f, randFloat() * 1.0f, randSigned() * 0.5f);
      glm::vec3 direction(randSigned(), randFloat() * 0.5f + 0.5f, randSigned());
      direction = glm::normalize(direction);
      spark.vel = direction * (kSparkSpeed * (0.7f + randFloat() * 0.6f));
      spark.life = kSparkLife * (0.8f + randFloat() * 0.4f);
      spark.maxLife = spark.life;
      spark.seed = randFloat();
      particles.push_back(spark);
    }
  }

  void PortalGate::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }

    if (alive_)
    {
      // Update position based on terrain height with offset
      transform_.position.y = params->terrainSampler(transform_.position.x, transform_.position.z) + kHeightOffset;
    }
  }

  void PortalGate::Render(const RenderContext &ctx)
  {
    if (!alive_)
    {
      return;
    }

    if (!model_)
    {
      static bool loggedMissingModel = false;
      if (!loggedMissingModel)
      {
        std::cerr << "[PortalGate] WARNING: Cannot render - model is null!" << std::endl;
        loggedMissingModel = true;
      }
      return;
    }

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, transform_.position);
    model = glm::scale(model, glm::vec3(modelScale_));
    model = glm::translate(model, -pivotOffset_);

    if (ctx.shadowPass)
    {
      if (!ctx.overrideShader)
      {
        return;
      }
      ctx.overrideShader->setMat4("model", model);
      model_->Draw(*ctx.overrideShader);
      return;
    }

    if (!shader_)
    {
      return;
    }

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);
    shader_->setMat4("lightSpaceMatrix", ctx.lightSpaceMatrix);
    shader_->setVec3("viewPos", ctx.viewPos);
    shader_->setVec3("lightPos", ctx.lightPos);
    shader_->setVec3("lightIntensity", ctx.lightIntensity);
    shader_->setBool("useBaseColor", useBaseColor_);
    if (useBaseColor_)
    {
      shader_->setVec3("baseColor", baseColor_);
    }

    glActiveTexture(GL_TEXTURE0 + kShadowMapTextureUnit);
    glBindTexture(GL_TEXTURE_2D, ctx.shadowMapTexture);
    shader_->setInt("shadowMap", kShadowMapTextureUnit);

    bool useSSAO = ctx.ssaoEnabled && ctx.ssaoTexture != 0;
    shader_->setBool("useSSAO", useSSAO);
    shader_->setVec2("screenSize", ctx.screenSize);
    shader_->setFloat("aoStrength", ctx.ssaoStrength);
    if (useSSAO)
    {
      glActiveTexture(GL_TEXTURE0 + kSSAOTexUnit);
      glBindTexture(GL_TEXTURE_2D, ctx.ssaoTexture);
      shader_->setInt("ssaoMap", kSSAOTexUnit);
    }

    shader_->setMat4("model", model);
    model_->Draw(*shader_);
  }

  void PortalGate::SetRenderResources(Shader *shader, Model *model, bool useBaseColor, const glm::vec3 &baseColor)
  {
    shader_ = shader;
    model_ = model;
    useBaseColor_ = useBaseColor;
    baseColor_ = baseColor;
  }

} // namespace mecha

