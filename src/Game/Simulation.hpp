#pragma once

#include "Tick.hpp"
#include "Entity.hpp"
#include "Renderer3D.hpp"
#include "SceneObject.hpp"
#include "DynamicArray.hpp"

namespace Ondine::Game {

// Structure handles all entities
class Simulation {
public:
  Simulation();

  virtual void init();
  virtual void tick(const Core::Tick &tick);

public:
  struct CreatedEntity {
    Entity &e;
    EntityHandle h; 
  };

  CreatedEntity createEntity();

  Entity &getEntity(EntityHandle hdl);

  template <typename T>
  void registerBehavior(uint32_t maxElements) {
    if (BehaviorID<T>::id == -1) {
      BehaviorID<T>::id = mBehaviors.size();
      mBehaviors.emplace_back(maxElements);
    }
  }

  template <typename T>
  DynamicArray<EntityHandle> &getEntitiesWith() {
    return mBehaviors[BehaviorID<T>::id];
  }

  template <typename T>
  void attachBehavior(EntityHandle handle) {
    auto &list = getEntitiesWith<T>();
    uint32_t idx = list.add();
    list[idx] = handle;
  }

private:
  static constexpr uint32_t kMaxEntityCount = 400;

  DynamicArray<Entity> mEntities;
  std::vector<DynamicArray<EntityHandle>> mBehaviors;
};

}
