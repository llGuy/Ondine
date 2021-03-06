#include <math.h>
#include "Log.hpp"
#include <assert.h>
#include "Utils.hpp"
#include "QuadTree.hpp"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

void QuadTree::init(uint16_t maxLOD) {
  mMaxLOD = maxLOD;
  mAllocatedNodeCount = 0;

  uint32_t nodeCount = 0;
  for (int i = 0; i <= mMaxLOD; ++i) {
    nodeCount += pow(4, i);
  }

  mDimensions = mArea = pow(2, mMaxLOD);
  mArea *= mArea;
  
  mNodeAllocator.init(nodeCount * sizeof(Node), sizeof(Node));
  mRoot = createNode(0, 0);
  mDeepestNodes.init(mArea);
  mDeepestNodes.size = 0;

  mDiffDelete.init(maxLOD * maxLOD);
  mDiffAdd.init(maxLOD * maxLOD);

  mDivisionCutoff = 1.0f;
}

void QuadTree::setInitialState(uint16_t minLevel) {
  populate(mRoot, minLevel);
}

uint32_t QuadTree::maxLOD() const {
  return mMaxLOD;
}

void QuadTree::setFocalPoint(const glm::vec2 &position) {
  // Clear the diff
  clearDiff();
  mDeepestNodes.size = 0;

  // Create the new nodes and push the modifications to the diff list
  populateDiff(mRoot, glm::vec2(0.0f), position);
}

void QuadTree::clearDiff() {
  for (auto add : mDiffAdd) {
    add.node->wasDiffed = 0;
  }

  mDiffDelete.clear();
  mDiffAdd.clear();
}

void QuadTree::clear() {
  clearDiff();

  mNodeAllocator.clear();
  mDeepestNodes.size = 0;

  mRoot = createNode(0, 0);
}

QuadTree::NodeInfo QuadTree::getNodeInfo(const glm::vec2 &position) const {
  NodeInfo nodeInfo = {};
  Node *deepestNode = getDeepestNode(position, &nodeInfo.offset);

  if (deepestNode) {
    nodeInfo.exists = true;
    nodeInfo.wasDiffed = deepestNode->wasDiffed;
    nodeInfo.level = deepestNode->level;
    nodeInfo.index = deepestNode->index;
    nodeInfo.offset = glm::vec2(deepestNode->offsetx, deepestNode->offsety);
    nodeInfo.size = glm::vec2(glm::pow(2.0f, (float)(mMaxLOD - nodeInfo.level)));
  }

  return nodeInfo;
}

uint32_t QuadTree::nodeCount() const {
  return mDeepestNodes.size;
}

QuadTree::NodeInfo QuadTree::getNodeInfo(uint32_t index) const {
  NodeInfo nodeInfo = {};
  const Node *deepestNode = mDeepestNodes[index];

  if (deepestNode) {
    nodeInfo.exists = true;
    nodeInfo.wasDiffed = deepestNode->wasDiffed;
    nodeInfo.level = deepestNode->level;
    nodeInfo.index = deepestNode->index;
    nodeInfo.offset = glm::vec2(deepestNode->offsetx, deepestNode->offsety);
    nodeInfo.size = glm::vec2(glm::pow(2.0f, (float)(mMaxLOD - nodeInfo.level)));
  }

  return nodeInfo;
}

QuadTree::Node *QuadTree::createNode(uint16_t level, uint16_t index) {
  Node *newNode = (Node *)mNodeAllocator.alloc();
  zeroMemory(newNode, sizeof(Node));
  newNode->level = level;
  newNode->index = index;

  ++mAllocatedNodeCount;

  return newNode;
}

void QuadTree::freeNode(Node *node) {
  for (int i = 0; i < 4; ++i) {
    if (node->children[i])
      freeNode(node->children[i]);
  }

  mNodeAllocator.free(node);

  --mAllocatedNodeCount;
}

void QuadTree::populateChildren(Node *node) {
  assert(node->level < mMaxLOD);
  for (int i = 0; i < 4; ++i) {
    node->children[i] = createNode(node->level + 1, i);
  }
}

void QuadTree::populate(Node *node, uint16_t maxLevel) {
  if (node->level < maxLevel) {
    for (int i = 0; i < 4; ++i) {
      node->children[i] = createNode(node->level + 1, i);
      populate(node->children[i], maxLevel);
    }
  }
}

void QuadTree::populate(
  Node *node,
  const glm::vec2 &offset,
  const glm::vec2 &position) {
  node->offsetx = offset.x;
  node->offsety = offset.y;

  if (node->level < mMaxLOD) {
    float scale = glm::pow(2.0f, (float)(mMaxLOD - node->level));
    glm::vec2 center = offset + glm::vec2(scale / 2.0f);

    float half = scale * mDivisionCutoff;
    float maxDist2 = half * half * 2.0f;
    glm::vec2 diff = position - center;
    float dist2 = glm::dot(diff, diff);

    if (dist2 <= maxDist2) {
      // Split the node
      for (int i = 0; i < 4; ++i) {
        node->children[i] = createNode(node->level + 1, i);
        glm::vec2 childOffset = offset + Node::INDEX_TO_OFFSET[i] * scale / 2.0f;
        populate(node->children[i], childOffset, position);
      }
    }
    else {
      mDeepestNodes[mDeepestNodes.size++] = node;
    }
  }
  else {
    mDeepestNodes[mDeepestNodes.size++] = node;
  }
}

void QuadTree::populateDiff(
  Node *node, const glm::vec2 &offset, const glm::vec2 &position) {
  node->offsetx = offset.x;
  node->offsety = offset.y;

  if (node->level < mMaxLOD) {
    float scale = glm::pow(2.0f, (float)(mMaxLOD - node->level));
    glm::vec2 center = offset + glm::vec2(scale / 2.0f);

    float half = scale * mDivisionCutoff;
    float maxDist2 = half * half * 2.0f;
    glm::vec2 diff = position - center;
    float dist2 = glm::dot(diff, diff);

    if (dist2 <= maxDist2) {
      // Does this node have children?
      if (node->children[0]) {
        // Don't add this to the list of diff
        for (int i = 0; i < 4; ++i) {
          glm::vec2 childOffset = offset + Node::INDEX_TO_OFFSET[i] * scale / 2.0f;
          populateDiff(node->children[i], childOffset, position);
        }
      }
      else {
        // Add this operation to the list of diff
        mDiffAdd.push({DiffOpType::Add, node});
        mDiffDelete.push({DiffOpType::Delete, node});
        node->wasDiffed = 1;

        // Split the node
        for (int i = 0; i < 4; ++i) {
          node->children[i] = createNode(node->level + 1, i);
          glm::vec2 childOffset = offset + Node::INDEX_TO_OFFSET[i] * scale / 2.0f;
          populate(node->children[i], childOffset, position);
        }
      }
    }
    else {
      if (node->children[0]) {
        mDiffDelete.push({DiffOpType::Delete, node});
        mDiffAdd.push({DiffOpType::Add, node});
        node->wasDiffed = 1;

        for (int i = 0; i < 4; ++i) {
          freeNode(node->children[i]);
          node->children[i] = nullptr;
        }
      }

      mDeepestNodes[mDeepestNodes.size++] = node;
    }
  }
  else {
    mDeepestNodes[mDeepestNodes.size++] = node;
  }
}

QuadTree::Node *QuadTree::getDeepestNode(
  const glm::vec2 &position,
  glm::vec2 *offsetOut) const {
  if (position.x < (float)mDimensions && position.y < (float)mDimensions) {
    Node *current = mRoot;
    glm::vec2 offset = glm::vec2(0);

    while (current->children[0]) {
      float size = glm::pow(2.0f, (float)(mMaxLOD - current->level));
      float innerBoundary = size / 2.0f;

      glm::vec2 diff = position - offset;

      if (diff.x < innerBoundary) {
        if (diff.y < innerBoundary) {
          current = current->children[0];
          offset = offset + Node::INDEX_TO_OFFSET[0] * innerBoundary;
        }
        else {
          current = current->children[1];
          offset = offset + Node::INDEX_TO_OFFSET[1] * innerBoundary;
        }
      }
      else {
        if (diff.y < innerBoundary) {
          current = current->children[2];
          offset = offset + Node::INDEX_TO_OFFSET[2] * innerBoundary;
        }
        else {
          current = current->children[3];
          offset = offset + Node::INDEX_TO_OFFSET[3] * innerBoundary;
        }
      }
    }

    if (offsetOut) {
      *offsetOut = offset;
    }

    return current;
  }
  else {
    return nullptr;
  }
}

Stack<QuadTree::NodeInfo, AllocationType::Linear> QuadTree::getDeepestNodesUnder(
  Node *node) {
  int width = pow(2, mMaxLOD - node->level);

  Stack<QuadTree::NodeInfo, AllocationType::Linear> list = {};
  list.init(width * width); // Theoretically the max amount of deep nodes

  getDeepestNodesUnderImpl(node, &list);

  return list;
}

void QuadTree::getDeepestNodesUnderImpl(
  Node *node, Stack<QuadTree::NodeInfo, AllocationType::Linear> *list) {
  int width = pow(2, mMaxLOD - node->level);

  if (!node->children[0]) {
    // This node doesn't have any children - it's the deepest
    list->push({
      true, (bool)node->wasDiffed, node->level, node->index,
      {node->offsetx, node->offsety},
      {width, width}
    });
  }
  else {
    for (int i = 0; i < 4; ++i) {
      getDeepestNodesUnderImpl(node->children[i], list);
    }
  }
}

}
