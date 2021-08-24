#include "Math.hpp"
#include "Terrain.hpp"
#include "Isosurface.hpp"

namespace Ondine::Graphics {

const glm::vec3 Isosurface::NORMALIZED_CUBE_VERTICES[8] = {
 glm::vec3(-0.5f, -0.5f, -0.5f),
  glm::vec3(+0.5f, -0.5f, -0.5f),
  glm::vec3(-0.5f, +0.5f, -0.5f),
  glm::vec3(+0.5f, +0.5f, -0.5f),
  glm::vec3(-0.5f, -0.5f, +0.5f),
  glm::vec3(+0.5f, -0.5f, +0.5f),
  glm::vec3(-0.5f, +0.5f, +0.5f),
  glm::vec3(+0.5f, +0.5f, +0.5f),
};

const glm::ivec3 Isosurface::NORMALIZED_CUBE_VERTEX_INDICES[8] = {
  glm::ivec3(0, 0, 0),
  glm::ivec3(1, 0, 0),
  glm::ivec3(0, 1, 0),
  glm::ivec3(1, 1, 0),
  glm::ivec3(0, 0, 1),
  glm::ivec3(1, 0, 1),
  glm::ivec3(0, 1, 1),
  glm::ivec3(1, 1, 1),
};

void Isosurface::init(const QuadTree &quadTree, VulkanContext &graphicsContext) {
  mGPUVerticesAllocator.init(
    10000,
    (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer,
    graphicsContext);

  mIsoGroups.init(1000);
  mIsoGroupIndices.init();

  mVertexPool = (IsoVertex *)malloc(
    sizeof(IsoVertex) * 10000 * 4096);

  auto maxUpdateCount = quadTree.mDimensions * quadTree.mDimensions * 2;
  mFullUpdates = new IsoGroup *[maxUpdateCount];
  mTransitionUpdates = new IsoGroup *[maxUpdateCount];
  mFullUpdateCount = 0;
  mTransitionUpdateCount = 0;
}

void Isosurface::bindToTerrain(const Terrain &terrain) {
  mScale = terrain.mTerrainScale;
  mSurfaceDensity = {30000};
}

void Isosurface::prepareForUpdate(QuadTree &quadTree, Terrain &terrain) {
  mScale = terrain.mTerrainScale;

  /* 
     Step #1: Figure out which chunk groups to delete and to create
     Step #2: Figure out which chunk groups to update the meshes for
     Step #3: Update those damn chunk groups
     (BONUS) Make step #2 distinguish between chunk groups which require
     just mesh transitions or everything to be updated
  */

  // Step #1
  for (auto deletion : quadTree.mDiffDelete) {
    QuadTree::Node *node = deletion.node;

    glm::ivec2 offset = {node->offsetx, node->offsety};
    // How many chunks does this node encompass
    int width = pow(2, quadTree.mMaxLOD - node->level);

    for (int z = offset.y; z < offset.y + width; ++z) {
      for (int x = offset.x; x < offset.x + width; ++x) {
        glm::ivec2 offset = quadTreeCoordsToChunk(quadTree, {x, z});
        IsoGroup *lowest = getFirstFlatIsoGroup(offset);

        if (lowest) {
          mFlatIsoGroupIndices.remove(hashFlatChunkCoord(offset));

          while (lowest) {
            // Delete the chunk group
            auto nextKey = lowest->next;
            freeIsoGroup(lowest);

            if (nextKey == INVALID_CHUNK_INDEX) {
              break;
            }
            else {
              lowest = mIsoGroups[nextKey];
            }
          }
        }
      }
    }
  }

  // Step #2
  for (auto addition : quadTree.mDiffAdd) {
    QuadTree::Node *node = addition.node;
    auto deepestNodes = quadTree.getDeepestNodesUnder(node);

    for (auto nodeInfo : deepestNodes) {
      glm::ivec2 offset = quadTreeCoordsToChunk(quadTree, nodeInfo.offset);
      int width = pow(2, quadTree.mMaxLOD - nodeInfo.level);

      // Generate chunk groups
      for (int z = offset.y; z < offset.y + width; ++z) {
        for (int x = offset.x; x < offset.x + width; ++x) {
          // For now we assume that the terrain doesn't get modified
          Chunk *current = terrain.getFirstFlatChunk(glm::ivec2(x, z));

          while (current) {
            glm::ivec3 groupCoord = getIsoGroupCoord(
              quadTree, current->chunkCoord);

            IsoGroup *group = getIsoGroup(groupCoord);
            group->level = nodeInfo.level;
            current->chunkGroupKey = group->key;

            int stride = (int)pow(2, quadTree.mMaxLOD - nodeInfo.level);
            float width = stride;
            
            glm::vec3 chunkCoordOffset = (glm::vec3)(
              current->chunkCoord - groupCoord);

            glm::vec3 start = (float)CHUNK_DIM * (chunkCoordOffset / width);

            for (int z = 0 ; z < CHUNK_DIM / stride; ++z) {
              for (int y = 0 ; y < CHUNK_DIM / stride; ++y) {
                for (int x = 0 ; x < CHUNK_DIM / stride; ++x) {
                  glm::ivec3 coord = glm::ivec3(x, y, z);

                  uint32_t dstVIndex = getVoxelIndex(coord + (glm::ivec3)start);
                  uint32_t srcVIndex = getVoxelIndex(coord * stride);

                  group->voxels[dstVIndex] = current->voxels[srcVIndex];
                }
              }
            }

            // Need to add transition updates for neighbouring chunk groups
            if (!group->pushedToFullUpdates) {
              mFullUpdates[mFullUpdateCount++] = group;
              group->pushedToFullUpdates = 1;
            }

            if (current->next == INVALID_CHUNK_INDEX) {
              current = nullptr;
            }
            else {
              current = terrain.mLoadedChunks[current->next];
            }
          }
        }
      }
    }
  }

  for (auto addition : quadTree.mDiffAdd) {
    QuadTree::Node *node = addition.node;
    int width = pow(2, quadTree.mMaxLOD - node->level);
    glm::ivec2 qtCoord = {node->offsetx, node->offsety};
    glm::ivec2 chunkCoord = quadTreeCoordsToChunk(quadTree, qtCoord);

    int components[] = {
      // Z, Z, X, X
      2, 2, 0, 0
    };

    glm::ivec2 offsets[] = {
      {-1, 0}, {width, 0}, {0, -1}, {0, width}
    };

    glm::ivec2 nav[] = {
      {0, 1}, {0, 1}, {1, 0}, {1, 0}
    };

    // Check neighbouring chunk groups
    for (int i = 0; i < 4; ++i) {
      glm::ivec2 adjNodeCoord = qtCoord + offsets[i];
      auto adjNodeInfo = quadTree.getNodeInfo((glm::vec2)adjNodeCoord);

      if (adjNodeInfo.exists && !adjNodeInfo.wasDiffed) {
        // (hmmm?) Transitions only need to be done for nodes with lower LOD
        glm::ivec2 adjChunkCoord = quadTreeCoordsToChunk(
          quadTree, adjNodeInfo.offset);

        int comp3D = components[i];
        int comp2D = comp3D / 2;

        while (
          adjChunkCoord[comp2D] < chunkCoord[comp2D] + width &&
          adjNodeInfo.exists) {
          int adjWidth = pow(2, quadTree.mMaxLOD - adjNodeInfo.level);

          // Do something
          IsoGroup *lowest = getFirstFlatIsoGroup(adjChunkCoord);
          while (lowest) {
            if (!lowest->pushedToFullUpdates &&
                !lowest->pushedToTransitionUpdates) {
              lowest->pushedToTransitionUpdates = 1;
              mTransitionUpdates[mTransitionUpdateCount++] = lowest;
            }
              
            auto nextKey = lowest->next;

            if (nextKey == INVALID_CHUNK_INDEX) {
              break;
            }
            else {
              lowest = mIsoGroups[nextKey];
            }
          }

          adjChunkCoord[comp2D] += adjWidth;
          adjNodeCoord[comp2D] += adjWidth;
          adjNodeInfo = quadTree.getNodeInfo((glm::vec2)adjNodeCoord);
        }
      }
    }
  }

  quadTree.clearDiff();

  Voxel surfaceDensity = {(uint16_t)30000};

  uint32_t vertexCounter = 0;

  for (int i = 0; i < mFullUpdateCount; ++i) {
    IsoGroup *group = mFullUpdates[i];

    uint32_t groupVertexCount = generateVertices(
      {terrain, quadTree, *group},
      mVertexPool + vertexCounter);

    group->vertexCount = groupVertexCount;
    group->verticesMem = mVertexPool + vertexCounter;

    vertexCounter += groupVertexCount;

    groupVertexCount = generateTransVoxelVertices(
      {terrain, quadTree, *group},
      mVertexPool + vertexCounter);

    group->transVoxelVertexCount = groupVertexCount;
    group->transVerticesMem = mVertexPool + vertexCounter;

    vertexCounter += groupVertexCount;
  }

  for (int i = 0; i < mTransitionUpdateCount; ++i) {
    IsoGroup *group = mTransitionUpdates[i];

    uint32_t groupVertexCount = generateTransVoxelVertices(
      {terrain, quadTree, *group},
      mVertexPool + vertexCounter);

    group->transVoxelVertexCount = groupVertexCount;
    group->transVerticesMem = mVertexPool + vertexCounter;

    vertexCounter += groupVertexCount;
  }
}

void Isosurface::syncWithGPU(const VulkanCommandBuffer &commandBuffer) {
  if (mFullUpdateCount) {
    for (int i = 0; i < mFullUpdateCount; ++i) {
      IsoGroup *group = mFullUpdates[i];

      group->pushedToFullUpdates = 0;
      group->pushedToTransitionUpdates = 0;

      if (group->vertices.size()) {
        // This chunk already has allocated memory
        mGPUVerticesAllocator.free(group->vertices);
      }

      if (group->transVoxelVertices.size()) {
        // This chunk already has allocated memory
        mGPUVerticesAllocator.free(group->transVoxelVertices);
      }

      if (group->vertexCount) {
        auto slot = mGPUVerticesAllocator.allocate(
          sizeof(IsoVertex) * group->vertexCount);

        slot.write(
          commandBuffer,
          group->verticesMem,
          sizeof(IsoVertex) * group->vertexCount);

        group->vertices = slot;
      }
      else {
        group->vertices = {};
      }

      if (group->transVoxelVertexCount) {
        auto slot = mGPUVerticesAllocator.allocate(
          sizeof(IsoVertex) * group->transVoxelVertexCount);

        slot.write(
          commandBuffer,
          group->transVerticesMem,
          sizeof(IsoVertex) * group->transVoxelVertexCount);

        group->transVoxelVertices = slot;
      }
      else {
        group->transVoxelVertices = {};
      }
    }

    mFullUpdateCount = 0;

    mGPUVerticesAllocator.debugLogState();
  }

  if (mTransitionUpdateCount) {
    for (int i = 0; i < mTransitionUpdateCount; ++i) {
      IsoGroup *group = mTransitionUpdates[i];
      
      group->pushedToFullUpdates = 0;
      group->pushedToTransitionUpdates = 0;

      if (group->transVoxelVertices.size()) {
        // This chunk already has allocated memory
        mGPUVerticesAllocator.free(group->transVoxelVertices);
      }

      if (group->transVoxelVertexCount) {
        auto slot = mGPUVerticesAllocator.allocate(
          sizeof(IsoVertex) * group->transVoxelVertexCount);

        slot.write(
          commandBuffer,
          group->transVerticesMem,
          sizeof(IsoVertex) * group->transVoxelVertexCount);

        group->transVoxelVertices = slot;
      }
      else {
        group->transVoxelVertices = {};
      }
    }

    mTransitionUpdateCount = 0;

    mGPUVerticesAllocator.debugLogState();
  }
}

uint32_t Isosurface::hashIsoGroupCoord(const glm::ivec3 &coord) const {
  struct {
    union {
      struct {
        uint32_t padding: 2;
        uint32_t x: 10;
        uint32_t y: 10;
        uint32_t z: 10;
      };
      uint32_t value;
    };
  } hasher;

  hasher.value = 0;

  hasher.x = *(uint32_t *)(&coord.x);
  hasher.y = *(uint32_t *)(&coord.y);
  hasher.z = *(uint32_t *)(&coord.z);

  return (uint32_t)hasher.value;
}

glm::ivec3 Isosurface::getIsoGroupCoord(
  const QuadTree &quadTree,
  const glm::ivec3 &chunkCoord) const {
  glm::ivec2 quadTreeCoord = glm::ivec2(chunkCoord.x, chunkCoord.z) +
    glm::ivec2(glm::pow(2, quadTree.maxLOD() - 1));

  QuadTree::NodeInfo node = quadTree.getNodeInfo(quadTreeCoord);

  node.offset -= glm::vec2(glm::pow(2.0f, quadTree.maxLOD() - 1));
  glm::ivec3 coord = glm::ivec3(node.offset.x, chunkCoord.y, node.offset.y);
  // Round down the nearest 2^node.level
  coord.y -= coord.y % (int)pow(2, quadTree.maxLOD() - node.level);

  return coord;
}

IsoGroup *Isosurface::getIsoGroup(const glm::ivec3 &coord) {
  uint32_t hash = hashIsoGroupCoord(coord);
  uint32_t *index = mIsoGroupIndices.get(hash);
    
  if (index) {
    // Chunk was already added
    return mIsoGroups[*index];
  }
  else {
    IsoGroup *group = flAlloc<IsoGroup>();

    auto key = mIsoGroups.add(group);

    zeroMemory(group);
    group->coord = coord;
    group->key = key;

    mIsoGroupIndices.insert(hash, key);
    addToFlatIsoGroupIndices(group);

    return group;
  }
}

void Isosurface::freeIsoGroup(IsoGroup *group) {
  uint32_t hash = hashIsoGroupCoord(group->coord);
  mIsoGroupIndices.remove(hash);

  mGPUVerticesAllocator.free(group->vertices);
  mGPUVerticesAllocator.free(group->transVoxelVertices);
  mIsoGroups.remove(group->key);

  // This is temporary - TODO: Add pre-allocated space for chunk groups
  flFree(group);
}

glm::ivec2 Isosurface::quadTreeCoordsToChunk(
  const QuadTree &quadTree,
  glm::ivec2 offset) const {
  return offset - glm::ivec2(pow(2, quadTree.maxLOD() - 1));
}

// One unit in offset = chunk coord. The origin of the quadtree is at 0,0
glm::ivec2 Isosurface::quadTreeCoordsToWorld(
  const QuadTree &quadTree,
  glm::ivec2 offset) const {
  offset -= glm::ivec2(pow(2, quadTree.maxLOD() - 1));
  offset *= CHUNK_DIM * mScale;
  return offset;
}

glm::vec2 Isosurface::worldToQuadTreeCoords(
  const QuadTree &quadTree,
  glm::vec2 offset) const {
  offset /= (CHUNK_DIM * mScale);
  offset += glm::vec2(glm::pow(2.0f, quadTree.maxLOD() - 1));
  return offset;
}

void Isosurface::addToFlatIsoGroupIndices(IsoGroup *group) {
  int x = group->coord.x, z = group->coord.z;
  uint32_t hash = hashFlatChunkCoord(glm::ivec2(x, z));
  uint32_t *index = mFlatIsoGroupIndices.get(hash);

  if (index) {
    IsoGroup *head = mIsoGroups[*index];
    group->next = head->key;
    *index = group->key;
  }
  else {
    mFlatIsoGroupIndices.insert(hash, group->key);
    group->next = INVALID_CHUNK_INDEX;
  }
}

IsoGroup *Isosurface::getFirstFlatIsoGroup(glm::ivec2 flatCoord) {
  uint32_t hash = hashFlatChunkCoord(flatCoord);
  uint32_t *index = mFlatIsoGroupIndices.get(hash);

  if (index) {
    return mIsoGroups[*index];
  }
  else {
    return nullptr;
  }
}

uint32_t Isosurface::generateVertices(
  GenIsoGroupVerticesParams params,
  IsoVertex *meshVertices) {
  const auto &terrain = params.terrain;
  const auto &quadTree = params.quadTree;
  const auto &group = params.group;

  int groupSize = pow(2, quadTree.mMaxLOD - group.level);
  int stride = groupSize;
  // glm::ivec3 groupCoord = group.coord + glm::ivec3(
  // pow(2, mQuadTree.mMaxLOD - 1));
  glm::ivec3 groupStart = group.coord * (int)CHUNK_DIM;

  // TODO: Optimise the getVoxel operation
  auto getVoxel = [&terrain, &groupSize, &groupStart](
    uint32_t x, uint32_t y, uint32_t z) {
    return terrain.getVoxel(groupStart + glm::ivec3(x, y, z) * groupSize);
  };

  auto getVoxelLOD = [&terrain, &groupSize, &groupStart, &stride](
    uint32_t x, uint32_t y, uint32_t z,
    // Offset
    uint32_t ox, uint32_t oy, uint32_t oz) {
    return terrain.getVoxel(
      groupStart + glm::ivec3(x, y, z) * groupSize +
      glm::ivec3(ox, oy, oz) * stride / 2);
  };

  uint32_t vertexCount = 0;

  for (uint32_t z = 1; z < CHUNK_DIM - 1; ++z) {
    for (uint32_t y = 1; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 1; x < CHUNK_DIM - 1; ++x) {
        Voxel voxelValues[8] = {
          group.voxels[getVoxelIndex(x,     y,     z)],
          group.voxels[getVoxelIndex(x + 1, y,     z)],
          group.voxels[getVoxelIndex(x,     y + 1, z)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z)],

          group.voxels[getVoxelIndex(x,     y,     z + 1)],
          group.voxels[getVoxelIndex(x + 1, y,     z + 1)],
          group.voxels[getVoxelIndex(x,     y + 1, z + 1)],
          group.voxels[getVoxelIndex(x + 1, y + 1, z + 1)],
        };

        updateVoxelCell(
          voxelValues, glm::ivec3(x, y, z),
          meshVertices, vertexCount);
      }
    }
  }

  return vertexCount;
}

uint32_t Isosurface::generateTransVoxelVertices(
  GenIsoGroupVerticesParams params,
  IsoVertex *meshVertices) {
  const auto &terrain = params.terrain;
  const auto &quadTree = params.quadTree;
  const auto &group = params.group;

  uint32_t vertexCount = 0;

  glm::ivec3 groupCoord = group.coord + glm::ivec3(
    pow(2, quadTree.mMaxLOD - 1));

  updateIsoGroupFace(
    params,
    0, 1, // Inner axis is X, outer axis is Y
    2, 1, // We are updating the positive Z face
    meshVertices, vertexCount);

  updateIsoGroupFace(
    params,
    0, 1, // Inner axis is X, outer axis is Y
    2, 0, // We are updating the negative Z face
    meshVertices, vertexCount);

  updateIsoGroupFace(
    params,
    1, 2, // Inner axis is Y, outer axis is Z
    0, 1, // We are updating the positive X face
    meshVertices, vertexCount);

  updateIsoGroupFace(
    params,
    1, 2, // Inner axis is Y, outer axis is Z
    0, 0, // We are updating the negative X face
    meshVertices, vertexCount);

  updateIsoGroupFace(
    params,
    0, 2, // Inner axis is x, outer axis is Z
    1, 1, // We are updating the positive Y face
    meshVertices, vertexCount);

  updateIsoGroupFace(
    params,
    0, 2, // Inner axis is X, outer axis is Z
    1, 0, // We are updating the negative Y face
    meshVertices, vertexCount);

  return vertexCount;
}

void Isosurface::updateIsoGroupFace(
  GenIsoGroupVerticesParams params,
  uint32_t primaryAxis, uint32_t secondAxis,
  uint32_t faceAxis, uint32_t side,
  IsoVertex *meshVertices, uint32_t &vertexCount) {
  const auto &terrain = params.terrain;
  const auto &quadTree = params.quadTree;
  const auto &group = params.group;

  int groupSize = pow(2, quadTree.mMaxLOD - group.level);
  glm::ivec3 groupStart = group.coord * (int)CHUNK_DIM;
  int stride = groupSize;

  glm::ivec3 groupCoordOffset = glm::ivec3(0);
  groupCoordOffset[faceAxis] = (int)side * 2 - 1;
  glm::ivec3 adjacentCoord = group.coord + glm::ivec3(
    pow(2, quadTree.mMaxLOD - 1));
  if (side == 1) {
    adjacentCoord += groupCoordOffset * groupSize;
  }
  else {
    adjacentCoord += groupCoordOffset;
  }

  QuadTree::NodeInfo adjacentNode = quadTree.getNodeInfo(
    glm::vec2(adjacentCoord.x, adjacentCoord.z));

  if (adjacentNode.exists) {
    static const glm::ivec3 offsets[8] = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
      {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1},
    };

    auto getVoxel = [&terrain, &groupSize, &groupStart](
      uint32_t x, uint32_t y, uint32_t z) {
      return terrain.getVoxel(groupStart + glm::ivec3(x, y, z) * groupSize);
    };

    auto getVoxelLOD = [&terrain, &groupSize, &groupStart, &stride](
      uint32_t x, uint32_t y, uint32_t z,
      // Offset
      uint32_t ox, uint32_t oy, uint32_t oz) {
      return terrain.getVoxel(
        groupStart + glm::ivec3(x, y, z) * groupSize +
        glm::ivec3(ox, oy, oz) * stride / 2);
    };

    if (adjacentNode.level <= group.level) {
      uint32_t d2 = (CHUNK_DIM - 1) * side;
      for (uint32_t d1 = 0; d1 < CHUNK_DIM; ++d1) {
        for (uint32_t d0 = 0; d0 < CHUNK_DIM; ++d0) {
          Voxel voxelValues[8] = {};

          for (int i = 0; i < 8; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i][primaryAxis];
            voxelCoord[secondAxis] = d1 + offsets[i][secondAxis];
            voxelCoord[faceAxis] = d2 + offsets[i][faceAxis];
          
            voxelValues[i] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          glm::ivec3 coord = {};
          coord[primaryAxis] = d0;
          coord[secondAxis] = d1;
          coord[faceAxis] = d2;

          updateVoxelCell(voxelValues, coord, meshVertices, vertexCount);
        }
      }
    }
    else {
      static const glm::ivec4 transOffsets[9] = {
        {0,0,0,0}, {0,0,1,0}, {1,0,0,0}, {0,0,0,1}, {0,0,1,1}, {1,0,0,1},
        {0,1,0,0}, {0,1,1,0}, {1,1,0,0}
      };

      uint32_t d2 = (CHUNK_DIM - 1) * side;
      for (uint32_t d1 = 0; d1 < CHUNK_DIM; ++d1) {
        for (uint32_t d0 = 0; d0 < CHUNK_DIM; ++d0) {
          Voxel voxelValues[8] = {};

          for (int i = 0; i < 8; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i][primaryAxis];
            voxelCoord[secondAxis] = d1 + offsets[i][secondAxis];
            voxelCoord[faceAxis] = d2 + offsets[i][faceAxis];

            voxelValues[i] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          Voxel transVoxels[13] = {};

          for (int i = 0; i < 9; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + transOffsets[i][0];
            voxelCoord[secondAxis] = d1 + transOffsets[i][1];
            voxelCoord[faceAxis] = d2 + side;

            glm::ivec3 halfCoord = {};
            halfCoord[primaryAxis] = transOffsets[i][2];
            halfCoord[secondAxis] = transOffsets[i][3];

            transVoxels[i] = getVoxelLOD(
              voxelCoord.x, voxelCoord.y, voxelCoord.z,
              halfCoord.x, halfCoord.y, halfCoord.z);
          }

          for (int i = 0; i < 4; ++i) {
            glm::ivec3 voxelCoord = {};
            voxelCoord[primaryAxis] = d0 + offsets[i][0];
            voxelCoord[secondAxis] = d1 + offsets[i][1];
            voxelCoord[faceAxis] = d2 + side;

            transVoxels[i + 9] = getVoxel(voxelCoord.x, voxelCoord.y, voxelCoord.z);
          }

          static const int CUBE_AXIS_BITS[3][2][4] = {
            {{0, 2, 4, 6}, {1, 3, 5, 7}},
            {{0, 1, 4, 5}, {2, 3, 6, 7}},
            {{0, 1, 2, 3}, {4, 5, 6, 7}}
          };

          for (int i = 0; i < 4; ++i) {
            transVoxels[i + 9].normalX = transVoxels[i + 9].normalX * 0.875 + voxelValues[CUBE_AXIS_BITS[faceAxis][(1 - side)][i]].normalX * 0.125;
            transVoxels[i + 9].normalY = transVoxels[i + 9].normalY * 0.875 + voxelValues[CUBE_AXIS_BITS[faceAxis][(1 - side)][i]].normalY * 0.125;
            transVoxels[i + 9].normalZ = transVoxels[i + 9].normalZ * 0.875 + voxelValues[CUBE_AXIS_BITS[faceAxis][(1 - side)][i]].normalZ * 0.125;
            voxelValues[CUBE_AXIS_BITS[faceAxis][side][i]].normalX = transVoxels[i + 9].normalX;
            voxelValues[CUBE_AXIS_BITS[faceAxis][side][i]].normalY = transVoxels[i + 9].normalY;
            voxelValues[CUBE_AXIS_BITS[faceAxis][side][i]].normalZ = transVoxels[i + 9].normalZ;
          }

          glm::ivec3 coord = {};
          coord[primaryAxis] = d0;
          coord[secondAxis] = d1;
          coord[faceAxis] = d2;

          glm::ivec3 axis = {};
          axis[faceAxis] = side * 2 - 1;

          updateTransVoxelCell(
            voxelValues, transVoxels,
            axis, coord, meshVertices, vertexCount);
        }
      }
    }
  }
}

#include "Transvoxel.inc"

void Isosurface::updateVoxelCell(
  Voxel *voxels,
  const glm::ivec3 &coord,
  IsoVertex *meshVertices,
  uint32_t &vertexCount) {
  uint8_t bitCombination = 0;
  for (uint32_t i = 0; i < 8; ++i) {
    bool isOverSurface = (voxels[i].density > mSurfaceDensity.density);
    bitCombination |= isOverSurface << i;
  }

  if (bitCombination == 0 || bitCombination == 0xFF) {
    return;
  }

  uint8_t cellClassIdx = regularCellClass[bitCombination];
  const RegularCellData &cellData = regularCellData[cellClassIdx];

  glm::vec3 vertices[8] = {};
  for (uint32_t i = 0; i < 8; ++i) {
    vertices[i] = NORMALIZED_CUBE_VERTICES[i] +
      glm::vec3(0.5f) + glm::vec3(coord);
  }

  IsoVertex *verts = STACK_ALLOC(IsoVertex, cellData.GetVertexCount());

  for (int i = 0; i < cellData.GetVertexCount(); ++i) {
    uint16_t nibbles = regularVertexData[bitCombination][i];

    if (nibbles == 0x0) {
      // Finished
      break;
    }

    uint8_t v0 = (nibbles >> 4) & 0xF;
    uint8_t v1 = nibbles & 0xF;

    float surfaceLevelF = (float)mSurfaceDensity.density;
    float voxelValue0 = (float)voxels[v0].density;
    float voxelValue1 = (float)voxels[v1].density;

    if (voxelValue0 > voxelValue1) {
      float tmp = voxelValue0;
      voxelValue0 = voxelValue1;
      voxelValue1 = tmp;

      uint8_t tmpV = v0;
      v0 = v1;
      v1 = tmpV;
    }

    float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
    glm::vec3 vertex = interpolate(
      vertices[v0], vertices[v1], interpolatedVoxelValues);

    glm::vec3 normal0 = glm::vec3(
      voxels[v0].normalX, voxels[v0].normalY, voxels[v0].normalZ) / 1000.0f;
    glm::vec3 normal1 = glm::vec3(
      voxels[v1].normalX, voxels[v1].normalY, voxels[v1].normalZ) / 1000.0f;

    glm::vec3 normal = interpolate(
      normal0, normal1, interpolatedVoxelValues);

    verts[i] = {vertex, normal};
  }

  for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
    int vertexIndex = cellData.vertexIndex[i];
    meshVertices[vertexCount++] = verts[vertexIndex];
  }
}

void Isosurface::updateTransVoxelCell(
  Voxel *voxels,
  Voxel *transVoxels,
  const glm::ivec3 &axis,
  const glm::ivec3 &coord,
  IsoVertex *meshVertices,
  uint32_t &vertexCount) {
  const float percentTrans = 0.125f;

  { // Normal mesh creation
    uint8_t bitCombination = 0;
    for (uint32_t i = 0; i < 8; ++i) {
      bool isOverSurface = (voxels[i].density > mSurfaceDensity.density);
      bitCombination |= isOverSurface << i;
    }

    if (bitCombination == 0 || bitCombination == 0xFF) {
      return;
    }

    uint8_t cellClassIdx = regularCellClass[bitCombination];
    const RegularCellData &cellData = regularCellData[cellClassIdx];

    glm::vec3 vertices[8] = {};
    for (uint32_t i = 0; i < 8; ++i) {
      vertices[i] = NORMALIZED_CUBE_VERTICES[i] +
        glm::vec3(0.5f) + glm::vec3(coord);
    }

    if (axis.x == -1) {
      vertices[0].x += percentTrans;
      vertices[2].x += percentTrans;
      vertices[4].x += percentTrans;
      vertices[6].x += percentTrans;
    }
    else if (axis.x == 1) {
      vertices[1].x -= percentTrans;
      vertices[3].x -= percentTrans;
      vertices[5].x -= percentTrans;
      vertices[7].x -= percentTrans;
    }
    else if (axis.z == -1) {
      vertices[0].z += percentTrans;
      vertices[1].z += percentTrans;
      vertices[2].z += percentTrans;
      vertices[3].z += percentTrans;
    }
    else if (axis.z == 1) {
      vertices[4].z -= percentTrans;
      vertices[5].z -= percentTrans;
      vertices[6].z -= percentTrans;
      vertices[7].z -= percentTrans;
    }

    IsoVertex *verts = STACK_ALLOC(IsoVertex, cellData.GetVertexCount());

    for (int i = 0; i < cellData.GetVertexCount(); ++i) {
      uint16_t nibbles = regularVertexData[bitCombination][i];

      if (nibbles == 0x0) {
        // Finished
        break;
      }

      uint8_t v0 = (nibbles >> 4) & 0xF;
      uint8_t v1 = nibbles & 0xF;

      float surfaceLevelF = (float)mSurfaceDensity.density;
      float voxelValue0 = (float)voxels[v0].density;
      float voxelValue1 = (float)voxels[v1].density;

      if (voxelValue0 > voxelValue1) {
        float tmp = voxelValue0;
        voxelValue0 = voxelValue1;
        voxelValue1 = tmp;

        uint8_t tmpV = v0;
        v0 = v1;
        v1 = tmpV;
      }

      float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
      glm::vec3 vertex = interpolate(
        vertices[v0], vertices[v1], interpolatedVoxelValues);

      glm::vec3 normal0 = glm::vec3(
        voxels[v0].normalX, voxels[v0].normalY, voxels[v0].normalZ) / 1000.0f;
      glm::vec3 normal1 = glm::vec3(
        voxels[v1].normalX, voxels[v1].normalY, voxels[v1].normalZ) / 1000.0f;

      glm::vec3 normal = interpolate(
        normal0, normal1, interpolatedVoxelValues);

      verts[i] = {vertex, normal};
    }

    for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
      int vertexIndex = cellData.vertexIndex[i];
      meshVertices[vertexCount++] = verts[vertexIndex];
    }
  }

  { // Transvoxel mesh creation
    uint32_t bitCombination = 0;

    bitCombination |= (transVoxels[0].density > mSurfaceDensity.density) << 0;
    bitCombination |= (transVoxels[1].density > mSurfaceDensity.density) << 1;
    bitCombination |= (transVoxels[2].density > mSurfaceDensity.density) << 2;
    bitCombination |= (transVoxels[3].density > mSurfaceDensity.density) << 7;
    bitCombination |= (transVoxels[4].density > mSurfaceDensity.density) << 8;
    bitCombination |= (transVoxels[5].density > mSurfaceDensity.density) << 3;
    bitCombination |= (transVoxels[6].density > mSurfaceDensity.density) << 6;
    bitCombination |= (transVoxels[7].density > mSurfaceDensity.density) << 5;
    bitCombination |= (transVoxels[8].density > mSurfaceDensity.density) << 4;

    if (bitCombination == 0 || bitCombination == 511) {
      return;
    }

    int detailed0, detailed1, nonDetailed, nonDetailedBit;

    if (axis.x == -1) {
      detailed0 = 1, detailed1 = 2, nonDetailed = 0, nonDetailedBit = 0;
    }
    else if (axis.x == 1) {
      detailed0 = 1, detailed1 = 2, nonDetailed = 0, nonDetailedBit = 1;
    }
    else if (axis.z == -1) {
      detailed0 = 0, detailed1 = 1, nonDetailed = 2, nonDetailedBit = 0;
    }
    else if (axis.z == 1) {
      detailed0 = 0, detailed1 = 1, nonDetailed = 2, nonDetailedBit = 1;
    }


    glm::vec3 vertices[13] = {};
    uint32_t counter = 0;

    for (int d1 = 0; d1 < 3; ++d1) {
      for (int d0 = 0; d0 < 3; ++d0) {
        vertices[counter][detailed0] = (float)d0 * 0.5f;
        vertices[counter][detailed1] = (float)d1 * 0.5f;
        vertices[counter][nonDetailed] = (float)nonDetailedBit;

        vertices[counter] += glm::vec3(coord);
        ++counter;
      }
    }

    for (int d1 = 0; d1 < 2; ++d1) {
      for (int d0 = 0; d0 < 2; ++d0) {
        vertices[counter][detailed0] = (float)d0;
        vertices[counter][detailed1] = (float)d1;
        vertices[counter][nonDetailed] = (float)(nonDetailedBit ^ 1);

        vertices[counter] += glm::vec3(coord);
        ++counter;
      }
    }

    if (nonDetailedBit) {
      if (axis[nonDetailed] == 1) {
        for (int i = 9; i < 13; ++i) {
          vertices[i][nonDetailed] += (1.0f - percentTrans);
        }
      }
    }
    else {
      if (axis[nonDetailed] == -1) {
        for (int i = 9; i < 13; ++i) {
          vertices[i][nonDetailed] -= (1.0f - percentTrans);
        }
      }
    }

    uint8_t cellClassIdx = transitionCellClass[bitCombination];
    const TransitionCellData &cellData = transitionCellData[cellClassIdx & 0x7F];

    IsoVertex *verts = STACK_ALLOC(IsoVertex, cellData.GetVertexCount());

    for (int i = 0; i < cellData.GetVertexCount(); ++i) {
      uint16_t nibbles = transitionVertexData[bitCombination][i];

      if (nibbles == 0x0) {
        break;
      }

      uint8_t v0 = (nibbles >> 4) & 0xF;
      uint8_t v1 = nibbles & 0xF;

      float surfaceLevelF = (float)mSurfaceDensity.density;
      float voxelValue0 = (float)transVoxels[v0].density;
      float voxelValue1 = (float)transVoxels[v1].density;

      if (voxelValue0 > voxelValue1) {
        float tmp = voxelValue0;
        voxelValue0 = voxelValue1;
        voxelValue1 = tmp;

        uint8_t tmpV = v0;
        v0 = v1;
        v1 = tmpV;
      }

      float interpolatedVoxelValues = lerp(voxelValue0, voxelValue1, surfaceLevelF);
    
      glm::vec3 vertex = interpolate(
        vertices[v0], vertices[v1], interpolatedVoxelValues);

      glm::vec3 normal0 = glm::vec3(
        transVoxels[v0].normalX,
        transVoxels[v0].normalY,
        transVoxels[v0].normalZ) / 1000.0f;

      glm::vec3 normal1 = glm::vec3(
        transVoxels[v1].normalX,
        transVoxels[v1].normalY,
        transVoxels[v1].normalZ) / 1000.0f;

      glm::vec3 normal = interpolate(
        normal0, normal1, interpolatedVoxelValues);

      glm::vec3 diff = vertex - (glm::vec3)coord;

      verts[i] = {vertex, normal};
    }

    if (cellClassIdx & 0x80) {
      for (int i = cellData.GetTriangleCount() * 3 - 1; i >= 0; --i) {
        int vertexIndex = cellData.vertexIndex[i];

        glm::vec3 diff = verts[vertexIndex].position - (glm::vec3)coord;

        meshVertices[vertexCount++] = verts[vertexIndex];
      }
    }
    else {
      for (int i = 0; i < cellData.GetTriangleCount() * 3; ++i) {
        int vertexIndex = cellData.vertexIndex[i];

        glm::vec3 diff = verts[vertexIndex].position - (glm::vec3)coord;

        meshVertices[vertexCount++] = verts[vertexIndex];
      }
    }
  }
}

NumericMap<IsoGroup *> &Isosurface::isoGroups() {
  return mIsoGroups;
}

}
