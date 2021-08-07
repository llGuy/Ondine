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

  mArea = pow(2, mMaxLOD);
  mArea *= mArea;
  
  mNodeAllocator.init(nodeCount * sizeof(Node), sizeof(Node));
  mRoot = createNode(0, 0);
  mDeepestNodes.init(mArea);
  mDeepestNodes.size = 0;
}

void QuadTree::setInitialState(uint16_t minLevel) {
  populate(mRoot, minLevel);
}

uint32_t QuadTree::maxLOD() const {
  return mMaxLOD;
}

void QuadTree::setFocalPoint(const glm::vec2 &position) {
  // Clear
  mNodeAllocator.clear();
  mDeepestNodes.size = 0;
  mAllocatedNodeCount = 0;

  mRoot = createNode(0, 0);
  populate(mRoot, glm::vec2(0.0f), position);
}

QuadTree::NodeInfo QuadTree::getNodeInfo(const glm::vec2 &position) const {
  NodeInfo nodeInfo = {};
  Node *deepestNode = getDeepestNode(position, &nodeInfo.offset);

  nodeInfo.level = deepestNode->level;
  nodeInfo.index = deepestNode->index;
  nodeInfo.offset = glm::vec2(deepestNode->offsetx, deepestNode->offsety);
  nodeInfo.size = glm::vec2(glm::pow(2.0f, (float)(mMaxLOD - nodeInfo.level)));

  return nodeInfo;
}

uint32_t QuadTree::nodeCount() const {
  return mDeepestNodes.size;
}

QuadTree::NodeInfo QuadTree::getNodeInfo(uint32_t index) const {
  NodeInfo nodeInfo = {};
  const Node *deepestNode = mDeepestNodes[index];

  nodeInfo.level = deepestNode->level;
  nodeInfo.index = deepestNode->index;
  nodeInfo.offset = glm::vec2(deepestNode->offsetx, deepestNode->offsety);
  nodeInfo.size = glm::vec2(glm::pow(2.0f, (float)(mMaxLOD - nodeInfo.level)));

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

    float half = scale / 2.0f;
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

QuadTree::Node *QuadTree::getDeepestNode(
  const glm::vec2 &position,
  glm::vec2 *offsetOut) const {
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

}
