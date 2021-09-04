#pragma once

#include "Event.hpp"

namespace Ondine::Core {

enum class TerrainTool : int {
  DensityPaintBrushAdd,
  DensityPaintBrushDestroy,
  ColorPaintBrush
};

struct EventTerrainToolChange : Event {
  EVENT_DEF(EventTerrainToolChange, Editor, TerrainToolChange);

  TerrainTool terrainTool;
};

}
