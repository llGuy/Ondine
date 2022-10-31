#include "Physics.hpp"
#include "Delegate.hpp"
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

void Simulation::tick(const Core::Tick &tick, const DelegateGeometryManager &geometryManager) {
  // For now just manually iterate over all physics behavior entities
  auto &physicsEntities = getEntitiesWith<PhysicsBehavior>();

  using Candidate = std::pair<EntityHandle, EntityHandle>;

  // Forward timestep for all entities
  for (auto hdl : physicsEntities) {
    auto &entity = getEntity(hdl);

    entity.position += entity.velocity * tick.dt;
  }

  const uint32_t kMaxCandidateCount = 100;

  uint32_t candidateCount = 0;
  Candidate *candidates = lnAllocv<Candidate>(kMaxCandidateCount);

  // Really stupid - will implement broadphase later with BVH
  for (auto a = physicsEntities.begin(); a != physicsEntities.end(); ++a) {
    // Create collision mesh for this entity
    auto &currentEntity = getEntity(*a);
    const Graphics::Geometry &geometry = geometryManager.getGeometry(currentEntity.geometryID);
    currentEntity.collisionMesh = Physics::createCollisionMesh(geometry, currentEntity);

    for (auto b = a; b != physicsEntities.end(); ++b) {
      if (*a != *b) {
        Candidate candidate = { *a, *b };

        candidates[candidateCount] = candidate;
        ++candidateCount;
      }
    }
  }

  // Process candidates
  for (int i = 0; i < candidateCount; ++i) {
    auto [hdl0, hdl1] = candidates[i];
    auto &entity0 = getEntity(hdl0);
    auto &entity1 = getEntity(hdl1);

    Physics::Collision collision = Physics::detectCollision(entity0.collisionMesh, entity1.collisionMesh);

    if (collision.bDetectedCollision) {
      // Assume all entities have the same mass
      glm::vec3 aToB = collision.normal;
      entity1.velocity = aToB;
      entity0.velocity = -aToB;

      LOG_INFO("There was a collision!");
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
