#include "ObjectiveSystem.h"
#include <sstream>

namespace mecha
{

  void ObjectiveSystem::Initialize(int totalPortals)
  {
    m_state = ObjectiveState{};
    m_state.totalPortals = totalPortals;
    m_state.type = ObjectiveType::DestroyPortals;
    m_bossSpawned = false;
  }

  void ObjectiveSystem::Reset()
  {
    int totalPortals = m_state.totalPortals;
    m_state = ObjectiveState{};
    m_state.totalPortals = totalPortals;
    m_state.type = ObjectiveType::DestroyPortals;
    m_bossSpawned = false;
  }

  void ObjectiveSystem::OnPortalDestroyed()
  {
    if (m_state.type == ObjectiveType::DestroyPortals)
    {
      m_state.portalsDestroyed++;
      UpdateObjective();
    }
  }

  void ObjectiveSystem::OnBossSpawned()
  {
    m_bossSpawned = true;
    // Objective should already be DefeatBoss by now
  }

  void ObjectiveSystem::OnBossDefeated()
  {
    if (m_state.type == ObjectiveType::DefeatBoss)
    {
      m_state.bossDefeated = true;
      UpdateObjective();
    }
  }

  void ObjectiveSystem::UpdateObjective()
  {
    if (m_state.type == ObjectiveType::DestroyPortals)
    {
      if (m_state.portalsDestroyed >= m_state.totalPortals)
      {
        // All portals destroyed, move to boss objective
        m_state.type = ObjectiveType::DefeatBoss;
      }
    }
    else if (m_state.type == ObjectiveType::DefeatBoss)
    {
      if (m_state.bossDefeated)
      {
        m_state.type = ObjectiveType::Complete;
        m_state.allObjectivesComplete = true;
      }
    }
  }

  std::string ObjectiveSystem::GetObjectiveText() const
  {
    std::ostringstream oss;

    switch (m_state.type)
    {
    case ObjectiveType::DestroyPortals:
      oss << "Destroy Portals: " << m_state.portalsDestroyed << "/" << m_state.totalPortals;
      break;

    case ObjectiveType::DefeatBoss:
      oss << "Defeat the Boss";
      break;

    case ObjectiveType::Complete:
      oss << "Victory!";
      break;
    }

    return oss.str();
  }

} // namespace mecha
