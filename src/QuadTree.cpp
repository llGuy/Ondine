#include <math.h>
#include "Log.hpp"
#include <assert.h>
#include "Utils.hpp"
#include "QuadTree.hpp"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

void QuadTree::init(uint16_t maxLOD) {
  mMaxLOD = maxLOD;

  uint32_t nodeCount = 0;
  for (int i = 0; i <= mMaxLOD; ++i) {
    nodeCount += pow(4, i);
  }
  
  mNodeAllocator.init(nodeCount * sizeof(Node), sizeof(Node));
  mRoot = createNode(0, 0);
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
  mRoot = createNode(0, 0);
  populate(mRoot, glm::vec2(0.0f), position);
}

QuadTree::Node *QuadTree::createNode(uint16_t level, uint16_t index) {
  Node *newNode = (Node *)mNodeAllocator.alloc();
  zeroMemory(newNode, sizeof(Node));
  newNode->level = level;
  newNode->index = index;
  return newNode;
}

void QuadTree::freeNode(Node *node) {
  for (int i = 0; i < 4; ++i) {
    if (node->children[i])
      freeNode(node->children[i]);
  }

  mNodeAllocator.free(node);
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
  if (node->level < mMaxLOD) {
    float scale = glm::pow(2.0f, (float)(mMaxLOD - node->level));
    glm::vec2 center = offset + glm::vec2(scale / 2.0f);

    float half = scale / 2.0f;
    float maxDist2 = half * half * 2.0f;
    glm::vec2 diff = position - center;
    float dist2 = glm::dot(diff, diff);

    if (dist2 <= maxDist2) {
      LOG_INFOV(
        "Split node at %s, level %d, centrer at %s, distance to focal point is %f over %f\n",
        glm::to_string(offset).c_str(), node->level,
        glm::to_string(center).c_str(), sqrt(dist2),
        sqrt(maxDist2));

      // Split the node
      for (int i = 0; i < 4; ++i) {
        node->children[i] = createNode(node->level + 1, i);
        glm::vec2 childOffset = offset + Node::INDEX_TO_OFFSET[i] * scale / 2.0f;
        populate(node->children[i], childOffset, position);
      }
    }
  }
}

}
