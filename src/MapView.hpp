#pragma once

#include "View.hpp"
#include "Scene.hpp"
#include "Delegate.hpp"
#include "RenderStage.hpp"
#include "EditorEvent.hpp"

namespace Ondine::View {

/* Forwards events to the map creation tool */
class MapView : public View {
public:
  MapView(
    Graphics::Renderer3D &renderer,
    Core::OnEventProc proc);

  ~MapView() override;

  void onPush(ViewPushParams &params) override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Core::Tick &tick, const Core::InputTracker &tracker) override;

  const Graphics::VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Core::Event *ev);
  void processEditorEvent(Core::Event *ev);
  void processInputEvent(Core::Event *ev);

  // For now process game input directly in the game view
  void processGameInput(
    const Core::Tick &tick, const Core::InputTracker &tracker);

private:
  Graphics::Scene *mMapScene;

  const Graphics::RenderStage &mMainRenderStage;
  Core::TerrainTool mTerrainTool;

  DelegateResize &mDelegateResize3D;
  Core::OnEventProc mOnEvent;
};

}
