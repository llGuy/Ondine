#include "Entity.hpp"
#include "Simulation.hpp"

namespace Ondine::Game {

Simulation::Simulation() 
  : mEntities(kMaxEntityCount) {

}

void Simulation::init() {
  registerBehavior<PhysicsBehavior>(100);
}

void Simulation::tick(const Core::Tick &tick) {
  // For now just manually iterate over all physics behavior entities
  auto &physicsEntities = getEntitiesWith<PhysicsBehavior>();

  for (auto handle : physicsEntities) {
    auto &entity = getEntity(handle);
    entity.position += entity.velocity * tick.dt;
  }
}

Simulation::CreatedEntity Simulation::createEntity() {
  uint32_t idx = mEntities.add();
  mEntities[idx].id = idx;
  return {mEntities[idx], idx};
}

Entity &Simulation::getEntity(EntityHandle hdl) {
  // TODO: Check if valid entity handle
  return mEntities[hdl];
}

}
