#pragma once

#include "yona_view.hpp"
#include "yona_render_stage.hpp"

namespace Yona {

class GameView : public View {
public:
  GameView(const RenderStage &gameRenderStage);
  ~GameView() override;

  void processEvents(ViewProcessEventsParams &) override;
  void render(ViewRenderParams &) override;

  const VulkanUniform &getOutput() const override;

private:
  const RenderStage &mGameRenderStage;
};

}
