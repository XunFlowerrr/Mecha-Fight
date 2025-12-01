#pragma once

#include <glm/glm.hpp>

#include "../../core/Entity.h"

namespace mecha
{
  /**
   * @brief Abstract base class for all enemy types
   * 
   * Provides a common interface for collision detection and damage handling.
   * Inheritance hierarchy: Entity -> Enemy -> EnemyDrone / TurretEnemy
   */
  class Enemy : public Entity
  {
  public:
    virtual ~Enemy() = default;

    /**
     * @brief Check if the enemy is currently alive
     * @return true if alive, false if dead
     */
    virtual bool IsAlive() const = 0;

    /**
     * @brief Get the collision radius of the enemy
     * @return Radius in world units
     */
    virtual float Radius() const = 0;

    /**
     * @brief Get the current position of the enemy
     * @return Position in world space
     */
    virtual const glm::vec3 &Position() const = 0;

    /**
     * @brief Apply damage to the enemy
     * @param amount Damage amount to apply
     */
    virtual void ApplyDamage(float amount) = 0;

    /**
     * @brief Get the current hit points of the enemy
     * @return Current HP value
     */
    virtual float HitPoints() const = 0;
  };

} // namespace mecha

