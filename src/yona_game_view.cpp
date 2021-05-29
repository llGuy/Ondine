#include "yona_game_view.hpp"

namespace Yona {

GameView::GameView(const RenderStage &gameRenderStage)
  : mGameRenderStage(gameRenderStage) {
  
}

GameView::~GameView() {
  
}

void GameView::processEvents(ViewProcessEventsParams &) {
  // Forward events to game state and game renderer
  // Tell the game to tick
}

void GameView::render(ViewRenderParams &) {
  // Doesn't need to do any rendering
}

const VulkanUniform &GameView::getOutput() const {
  return mGameRenderStage.uniform();
}

}
