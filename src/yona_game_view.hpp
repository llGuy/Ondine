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

private:
  const RenderStage &mGameRenderStage;
  // Allows to call Renderer3D::resize
  DelegateResize &mDelegateResize3D;
};

}
