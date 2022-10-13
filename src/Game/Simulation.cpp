#include "Log.hpp"
#include "Entity.hpp"
#include "Memory.hpp"
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

  using Candidate = std::pair<EntityHandle, EntityHandle>;

  // Forward timestep for all entities
  for (auto hdl : physicsEntities) {
    auto &entity = getEntity(hdl);

    entity.position += entity.velocity * tick.dt;
  }

  uint32_t candidateCount = 0;
  Candidate *candidates = lnAllocv<Candidate>(0);
  freezeLinearAllocator();

  // Really stupid
  for (auto a = physicsEntities.begin(); a != physicsEntities.end(); ++a) {
    for (auto b = a; b != physicsEntities.end(); ++b) {
      if (*a != *b) {
        Candidate candidate = { *a, *b };

        unfreezeLinearAllocator();
        *lnAllocv<Candidate>(1) = candidate;
        freezeLinearAllocator();

        ++candidateCount;
      }
    }
  }

  unfreezeLinearAllocator();

  // Process candidates
  for (int i = 0; i < candidateCount; ++i) {
    auto [hdl0, hdl1] = candidates[i];
    auto &entity0 = getEntity(hdl0);
    auto &entity1 = getEntity(hdl1);

    AABB aabb0 = constructTransform(entity0) * entity0.aabb;
    AABB aabb1 = constructTransform(entity1) * entity1.aabb;

    if (aabb0.overlaps(aabb1)) {
      glm::vec3 aToB = entity1.position - entity0.position;
      entity1.velocity = aToB;
      entity0.velocity = -aToB;

      // We have a collision!
      LOG_INFO("There was a collision!\n");
    }
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
