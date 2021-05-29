#pragma once

#include "yona_view.hpp"
#include "yona_render_stage.hpp"
#include "yona_delegate_resize.hpp"

namespace Yona {

class GameView : public View {
public:
  GameView(
    const RenderStage &gameRenderStage,
    DelegateResize &delegateResize3D);
  ~GameView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  const VulkanUniform &getOutput() const override;

private:
  void processRendererEvent(Event *ev);
  void processDeferredEvents(VulkanContext &graphicsContext);

private:
  static constexpr uint32_t MAX_EVENT_FUNCTORS = 20;

  struct DeferredEventProcParams {
    GameView *gameView;
    VulkanContext &graphicsContext;
    void *data;
  };

  static void handleResize(DeferredEventProcParams &params);

  struct DeferredEventFunctor {
    void (*proc)(DeferredEventProcParams &params);
    void *data;
  };

private:
  const RenderStage &mGameRenderStage;
  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
  DeferredEventFunctor mDeferredEvents[MAX_EVENT_FUNCTORS];
  uint32_t mDeferredEventCount;
};

}
