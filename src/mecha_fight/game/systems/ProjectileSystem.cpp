#include "ProjectileSystem.h"

#include <algorithm>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../entities/Enemy.h"
#include "../entities/MechaPlayer.h"
#include "../entities/GodzillaEnemy.h"
#include "../ui/DeveloperOverlayUI.h"
#include "../audio/SoundManager.h"
#include <learnopengl/shader_m.h>

namespace mecha
{
  namespace
  {
    constexpr float kPlayerHitRadius = 0.35f;
    constexpr float kPlayerDamage = 12.0f;
    constexpr float kEnemyDamage = 20.0f;
    constexpr float kPlayerBulletLife = 3.0f;
    constexpr float kEnemyBulletLife = 5.0f;
  }

  void ProjectileSystem::SpawnPlayerShot(const glm::vec3 &position, const glm::vec3 &velocity)
  {
    bullets_.push_back({position, velocity, kPlayerBulletLife, false, 0.06f});
  }

  void ProjectileSystem::SpawnEnemyShot(const glm::vec3 &position, const glm::vec3 &velocity)
  {
    bullets_.push_back({position, velocity, kEnemyBulletLife, true, 0.08f});
  }

  void ProjectileSystem::SpawnEnemyShot(const glm::vec3 &position, const glm::vec3 &velocity, float size)
  {
    bullets_.push_back({position, velocity, kEnemyBulletLife, true, size});
  }

  const std::vector<Bullet> &ProjectileSystem::Bullets() const
  {
    return bullets_;
  }

  void ProjectileSystem::Update(const UpdateContext &ctx)
  {
    const auto *params = static_cast<const UpdateParams *>(GetFramePayload());
    if (!params)
    {
      return;
    }

    MechaPlayer *player = params->player;
    const float deltaTime = ctx.deltaTime;

    for (auto &b : bullets_)
    {
      b.pos += b.vel * deltaTime;
      b.life -= deltaTime;
    }

    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [&](const Bullet &b)
                                  {
      if (b.life <= 0.0f)
      {
        return true;
      }

      if (b.fromEnemy)
      {
        if (!player)
        {
          return false;
        }

        const auto &move = player->Movement();
        if (glm::distance(b.pos, move.position) < kPlayerHitRadius)
        {
          if (!(params->overlay && params->overlay->godMode))
          {
            player->TakeDamage(kPlayerDamage);
          }

          // Play impact sound
          if (params->soundManager)
          {
            params->soundManager->PlaySound3D("PROJECTILE_IMPACT", b.pos);
          }

          return true;
        }
      }
      else
      {
        // Check all enemies for collision (unified loop)
        for (Enemy *enemy : params->enemies)
        {
          if (enemy && enemy->IsAlive())
      {
            // Special handling for GodzillaEnemy - check guns first
            GodzillaEnemy *godzilla = dynamic_cast<GodzillaEnemy *>(enemy);
            if (godzilla)
            {
              // Check if bullet hits a gun
              int gunIndex = godzilla->GetGunAtPosition(b.pos, 0.0f);
              if (gunIndex >= 0)
              {
                godzilla->ApplyDamageToGun(gunIndex, kEnemyDamage);
                return true;
              }
              // If no gun hit, check body
              if (glm::distance(b.pos, enemy->Position()) < enemy->Radius())
              {
                enemy->ApplyDamage(kEnemyDamage);
                return true;
              }
            }
            else
            {
              // Regular enemy collision check
              if (glm::distance(b.pos, enemy->Position()) < enemy->Radius())
        {
          enemy->ApplyDamage(kEnemyDamage);

          // Play impact sound
          if (params->soundManager)
          {
            params->soundManager->PlaySound3D("PROJECTILE_IMPACT", b.pos);
          }

          return true;
              }
            }
          }
        }
      }

      return false; }),
                   bullets_.end());
  }

  void ProjectileSystem::Render(const RenderContext &ctx)
  {
    if (ctx.shadowPass)
    {
      return;
    }

    if (bullets_.empty() || !shader_ || sphereVAO_ == 0 || sphereIndexCount_ == 0)
    {
      return;
    }

    shader_->use();
    shader_->setMat4("projection", ctx.projection);
    shader_->setMat4("view", ctx.view);

    glBindVertexArray(sphereVAO_);

    for (const auto &b : bullets_)
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, b.pos);
      model = glm::scale(model, glm::vec3(b.size));
      shader_->setMat4("model", model);
      shader_->setVec4("color", b.fromEnemy ? glm::vec4(1.0f, 0.15f, 0.15f, 1.0f)
                                            : glm::vec4(0.2f, 1.0f, 1.0f, 1.0f));
      glDrawElements(GL_TRIANGLES, sphereIndexCount_, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
  }

  void ProjectileSystem::SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount)
  {
    shader_ = shader;
    sphereVAO_ = sphereVAO;
    sphereIndexCount_ = sphereIndexCount;
  }

} // namespace mecha
