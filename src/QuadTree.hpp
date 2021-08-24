#pragma once

#include "Stack.hpp"
#include "Buffer.hpp"
#include <glm/glm.hpp>
#include "ArenaAllocator.hpp"

namespace Ondine::View {

class EditorView;

}

namespace Ondine::Graphics {

// Quad tree structure which we use for the terrain system
class QuadTree {
public:
  void init(uint16_t maxLOD);
  void setInitialState(uint16_t minLevel);

  uint32_t maxLOD() const;
  // This position needs to be in quadtree space
  void setFocalPoint(const glm::vec2 &position);

  struct NodeInfo {
    bool exists;
    bool wasDiffed;
    
    uint16_t level;
    uint16_t index;
    glm::vec2 offset;
    glm::vec2 size;
  };

  // In quadtree space
  NodeInfo getNodeInfo(const glm::vec2 &position) const;

  // Deepest nodes
  uint32_t nodeCount() const;
  NodeInfo getNodeInfo(uint32_t index) const;

  void clearDiff();

private:
  struct Node {
    static constexpr glm::vec2 INDEX_TO_OFFSET[4] = {
      glm::vec2(0.0f, 0.0f),
      glm::vec2(0.0f, 1.0f),
      glm::vec2(1.0f, 0.0f),
      glm::vec2(1.0f, 1.0f)
    };

    uint16_t level;
    // Index into the children array
    uint8_t index;
    uint8_t wasDiffed;
    Node *children[4];

    uint16_t offsetx, offsety;
  };

  enum class DiffOpType {
    Add, // Need to process new chunk groups
    Delete // Need to remove chunk groups
  };

  struct DiffOp {
    DiffOpType type;
    Node *node;
  };

  Node *createNode(uint16_t level, uint16_t index);
  void freeNode(Node *node);
  void populateChildren(Node *node);
  void populate(Node *node, uint16_t maxLevel);
  void populate(Node *node, const glm::vec2 &offset, const glm::vec2 &position);
  Node *getDeepestNode(
    const glm::vec2 &position, glm::vec2 *offset = nullptr) const;

  Stack<NodeInfo, AllocationType::Linear> getDeepestNodesUnder(Node *node);
  void getDeepestNodesUnderImpl(
    Node *node, Stack<NodeInfo, AllocationType::Linear> *list = nullptr);

  void populateDiff(
    Node *node, const glm::vec2 &offset, const glm::vec2 &position);

private:
  Node *mRoot;
  uint32_t mArea;
  uint16_t mMaxLOD;
  uint32_t mDimensions;
  Array<Node *> mDeepestNodes;
  uint32_t mAllocatedNodeCount;
  ArenaAllocator mNodeAllocator;

  // Factor of the side of the node
  float mDivisionCutoff;

  glm::ivec2 mFocalPoint;

  // Diff
  Stack<DiffOp> mDiffDelete;
  Stack<DiffOp> mDiffAdd;

  friend class Terrain;
  friend class TerrainRenderer;
  friend class Isosurface;
  friend class View::EditorView;
};

}
