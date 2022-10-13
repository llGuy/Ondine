#include "EntityCamera.hpp"

namespace Ondine::Game {

void EntityCamera::init() {
  mAttachedEntity = kInvalidEntityHandle;
}

void EntityCamera::setCameraType(EntityCameraType type) {
  mType = type;
}

void EntityCamera::tick(const Core::Tick &tick, Simulation &sim) {
  Entity &attached = sim.getEntity(mAttachedEntity);

  mPosition = attached.position;
  mViewDirection = attached.viewDirection;
  mUp = kGlobalUp;
}

EntityHandle &EntityCamera::getAttachedEntity() {
  return mAttachedEntity;
}

}
