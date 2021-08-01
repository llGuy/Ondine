#pragma once

#include <glm/glm.hpp>
#include "ArenaAllocator.hpp"

namespace Ondine::Graphics {

// Quad tree structure which we use for the terrain system
class QuadTree {
public:
  void init(uint16_t maxLOD);
  void setInitialState(uint16_t minLevel);

  uint32_t maxLOD() const;
  // This position needs to be in quadtree space
  void setFocalPoint(const glm::vec2 &position);

private:
  struct Node {
    static constexpr glm::vec2 INDEX_TO_OFFSET[4] = {
      glm::vec2(0.0f, 0.0f),
      glm::vec2(0.0f, 0.0f),
      glm::vec2(1.0f, 0.0f),
      glm::vec2(1.0f, 0.0f)
    };

    uint16_t level;
    // Index into the children array
    uint16_t index;
    Node *children[4];
  };

  Node *createNode(uint16_t level, uint16_t index);
  void freeNode(Node *node);
  void populateChildren(Node *node);
  void populate(Node *node, uint16_t maxLevel);
  void populate(Node *node, const glm::vec2 &offset, const glm::vec2 &position);

private:
  Node *mRoot;
  uint32_t mArea;
  uint16_t mMaxLOD;
  uint32_t mDimensions;
  ArenaAllocator mNodeAllocator;

  friend class TerrainRenderer;
};

}
