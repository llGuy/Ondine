#pragma once

#include "IO.hpp"
#include "Tick.hpp"
#include "Delegate.hpp"
#include "Simulation.hpp"
#include "EntityCamera.hpp"
#include "InputTranslator.hpp"
#include "EntityController.hpp"

namespace Ondine::Game {

class Game : public DelegateTickable, public DelegateTrackInput {
public:
  // Initialize ALL non-rendering game state
  void init(const DelegateGeometryManager &geometryManager);

  // Initialize game rendering state
  void initGameRendering(Graphics::Renderer3D &);

  // Update game (not just the current level that is being played)
  void tick(const Core::Tick &tick) override;

  // Process input - translates to game actions
  void trackInput(
    const Core::Tick &tick,
    const Core::InputTracker &inputTracker) override;

  struct Viewer {
    glm::vec3 wPosition;
    glm::vec3 wViewDirection;
    glm::vec3 wUp;
  };

  Viewer getViewerInfo();

  Graphics::Scene *getScene();

private:
  void makeEntityRenderable(Entity &entity, const char *renderMethod);
  void updateEntitySceneObject(Entity &entity);

private:
  // Simulation handles all entity management
  Simulation mSimulation;

  // Entity camera attaches to an entity from simulation
  EntityCamera mEntityCamera;
  
  // Entity controller attaches to the entity that we control with keyboard/mouse
  EntityController mEntityController;
  InputTranslator mInputTranslator;

  EntityHandle mFloor;
  EntityHandle mCubeA;
  EntityHandle mCubeB;

  float mRotation;

  // Current rendering scene
  Graphics::Scene *mScene;
  // This is gonna be tied to the renderer for now
  const DelegateGeometryManager *mGeometryManager;
};

}
