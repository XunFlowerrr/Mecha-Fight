#pragma once

#include <vector>

#include "../../core/Entity.h"
#include "../GameplayTypes.h"

class Shader;

namespace mecha
{
  class MechaPlayer;
  class Enemy;
  struct DeveloperOverlayState;

  class ProjectileSystem : public Entity
  {
  public:
    struct UpdateParams
    {
      MechaPlayer *player{nullptr};
      std::vector<Enemy *> enemies;
      DeveloperOverlayState *overlay{nullptr};
      class SoundManager *soundManager{nullptr};
    };

    void Update(const UpdateContext &ctx) override;

    void SpawnPlayerShot(const glm::vec3 &position, const glm::vec3 &velocity);
    void SpawnEnemyShot(const glm::vec3 &position, const glm::vec3 &velocity);
    void SpawnEnemyShot(const glm::vec3 &position, const glm::vec3 &velocity, float size);

    const std::vector<Bullet> &Bullets() const;

    void Render(const RenderContext &ctx) override;
    void SetRenderResources(Shader *shader, unsigned int sphereVAO, unsigned int sphereIndexCount);

  private:
    std::vector<Bullet> bullets_{};
    Shader *shader_{nullptr};
    unsigned int sphereVAO_{0};
    unsigned int sphereIndexCount_{0};
  };

} // namespace mecha
