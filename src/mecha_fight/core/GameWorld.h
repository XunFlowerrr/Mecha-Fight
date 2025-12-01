#pragma once

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "Entity.h"

namespace mecha
{

  class GameWorld
  {
  public:
    GameWorld() = default;

    template <typename TEntity, typename... TArgs>
    std::shared_ptr<TEntity> CreateEntity(TArgs &&...args)
    {
      static_assert(std::is_base_of<Entity, TEntity>::value, "TEntity must derive from Entity");
      auto entity = std::make_shared<TEntity>(std::forward<TArgs>(args)...);
      entities_.push_back(entity);
      return entity;
    }

    void AddEntity(const std::shared_ptr<Entity> &entity);

    void ForEachEntity(const std::function<void(Entity &)> &fn);

    void Update(const UpdateContext &ctx);
    void FixedUpdate(const UpdateContext &ctx);
    void Render(const RenderContext &ctx);

    const std::vector<std::shared_ptr<Entity>> &Entities() const { return entities_; }

  private:
    std::vector<std::shared_ptr<Entity>> entities_;
  };

} // namespace mecha
