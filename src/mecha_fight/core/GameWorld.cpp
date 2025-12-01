#include "GameWorld.h"
#include <iostream>
#include <typeinfo>

namespace mecha
{

  void GameWorld::AddEntity(const std::shared_ptr<Entity> &entity)
  {
    if (entity)
    {
      entities_.push_back(entity);
    }
  }

  void GameWorld::ForEachEntity(const std::function<void(Entity &)> &fn)
  {
    for (auto &entity : entities_)
    {
      if (entity)
      {
        fn(*entity);
      }
    }
  }

  void GameWorld::Update(const UpdateContext &ctx)
  {
    int index = 0;
    for (auto &entity : entities_)
    {
      if (entity)
      {
        Entity &entityRef = *entity;
        entityRef.Update(ctx);
      }
      index++;
    }
  }

  void GameWorld::FixedUpdate(const UpdateContext &ctx)
  {
    for (auto &entity : entities_)
    {
      if (entity)
      {
        entity->FixedUpdate(ctx);
      }
    }
  }

  void GameWorld::Render(const RenderContext &ctx)
  {
    for (auto &entity : entities_)
    {
      if (entity)
      {
        entity->Render(ctx);
      }
    }
  }

} // namespace mecha
