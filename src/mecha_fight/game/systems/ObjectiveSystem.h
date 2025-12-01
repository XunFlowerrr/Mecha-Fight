#pragma once

#include <string>
#include <functional>

namespace mecha
{

  /**
   * @brief Manages game objectives and progression
   *
   * Objectives:
   * 1. Destroy all portals (tracks count)
   * 2. Defeat the boss
   */
  class ObjectiveSystem
  {
  public:
    enum class ObjectiveType
    {
      DestroyPortals,
      DefeatBoss,
      Complete
    };

    struct ObjectiveState
    {
      ObjectiveType type = ObjectiveType::DestroyPortals;
      int portalsDestroyed = 0;
      int totalPortals = 2;
      bool bossDefeated = false;
      bool allObjectivesComplete = false;
    };

    ObjectiveSystem() = default;

    /**
     * @brief Initialize the objective system
     * @param totalPortals Number of portals to destroy
     */
    void Initialize(int totalPortals);

    /**
     * @brief Reset objectives to initial state
     */
    void Reset();

    /**
     * @brief Called when a portal is destroyed
     */
    void OnPortalDestroyed();

    /**
     * @brief Called when the boss spawns
     */
    void OnBossSpawned();

    /**
     * @brief Called when the boss is defeated
     */
    void OnBossDefeated();

    /**
     * @brief Get current objective state
     */
    const ObjectiveState &GetState() const { return m_state; }

    /**
     * @brief Get current objective display text
     */
    std::string GetObjectiveText() const;

    /**
     * @brief Check if all objectives are complete
     */
    bool IsComplete() const { return m_state.allObjectivesComplete; }

    /**
     * @brief Get current objective type
     */
    ObjectiveType GetCurrentObjective() const { return m_state.type; }

  private:
    void UpdateObjective();

    ObjectiveState m_state;
    bool m_bossSpawned = false;
  };

} // namespace mecha
