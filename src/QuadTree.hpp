#pragma once

#include "ArenaAllocator.hpp"

namespace Ondine::Graphics {

// Quad tree structure which we use for the terrain system
class QuadTree {
public:
  void init(uint16_t maxLOD);
  void setInitialState(uint16_t minLevel);

  uint32_t maxLOD() const;

private:
  struct Node {
    uint16_t level;
    // Index into the children array
    uint16_t index;
    Node *children[4];
  };

  Node *createNode(uint16_t level, uint16_t index);
  void freeNode(Node *node);
  void populateChildren(Node *node);
  void populate(Node *node, uint16_t maxLevel);

private:
  Node *mRoot;
  uint32_t mArea;
  uint16_t mMaxLOD;
  uint32_t mDimensions;
  ArenaAllocator mNodeAllocator;

  friend class TerrainRenderer;
};

}
