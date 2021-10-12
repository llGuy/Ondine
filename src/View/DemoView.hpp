#pragma once

#include "View.hpp"
#include "Scene.hpp"
#include "Renderer3D.hpp"
#include "RenderStage.hpp"

namespace Ondine::View {

/* Forwards events to the game state */
class DemoView : public View {
public:
  DemoView(
    Graphics::Renderer3D &renderer,
    Core::OnEventProc proc);

  ~DemoView() override;

  void onPush(ViewPushParams &params) override;
  void onPop(ViewPushParams &) override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  FocusedView trackInput(
    const Core::Tick &tick, const Core::InputTracker &tracker) override;

  const Graphics::VulkanUniform &getOutput() const override;

private:
  void processGraphicsEvent(Core::Event *ev);
  void processInputEvent(Core::Event *ev);

  // For now process game input directly in the game view
  void processGameInput(
    const Core::Tick &tick, const Core::InputTracker &tracker);

private:
  const Graphics::RenderStage &mGameRenderStage;
  Graphics::Scene *mDemoScene;

  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  Core::OnEventProc mOnEvent;

  // Just have a reference to this for easy access. This is just a demo
  Graphics::Renderer3D &mRenderer3D;
};

}
