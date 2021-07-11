#include "Math.hpp"
#include "Camera.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

const glm::vec3 Terrain::NORMALIZED_CUBE_VERTICES[8] = {
  glm::vec3(-0.5f, -0.5f, -0.5f),
  glm::vec3(+0.5f, -0.5f, -0.5f),
  glm::vec3(+0.5f, -0.5f, +0.5f),
  glm::vec3(-0.5f, -0.5f, +0.5f),
  glm::vec3(-0.5f, +0.5f, -0.5f),
  glm::vec3(+0.5f, +0.5f, -0.5f),
  glm::vec3(+0.5f, +0.5f, +0.5f),
  glm::vec3(-0.5f, +0.5f, +0.5f)
};

const glm::ivec3 Terrain::NORMALIZED_CUBE_VERTEX_INDICES[8] = {
  glm::ivec3(0, 0, 0),
  glm::ivec3(1, 0, 0),
  glm::ivec3(1, 0, 1),
  glm::ivec3(0, 0, 1),
  glm::ivec3(0, 1, 0),
  glm::ivec3(1, 1, 0),
  glm::ivec3(1, 1, 1),
  glm::ivec3(0, 1, 1)
};

void Terrain::init() {
  mTerrainScale = 1.0f;
  mChunkWidth = mTerrainScale * (float)CHUNK_DIM;

  mChunkIndices.init();
  mLoadedChunks.init(MAX_CHUNKS);

  mTemporaryVertices = flAllocv<ChunkVertex>(CHUNK_MAX_VERTICES);
}

Chunk *Terrain::getChunk(const glm::ivec3 &coord) {
  uint32_t hash = hashChunkCoord(coord);
  uint32_t *index = mChunkIndices.get(hash);
    
  if (index) {
    // Chunk was already added
    return mLoadedChunks[*index];
  }
  else {
    uint32_t i = mLoadedChunks.size++;
    Chunk *&chunk = mLoadedChunks[i];
    chunk = flAlloc<Chunk>();

    memset(chunk, 0, sizeof(Chunk));
    chunk->chunkCoord = coord;
    chunk->chunkStackIndex = i;

    mChunkIndices.insert(hash, i);

    return chunk;
  }
}

Chunk *Terrain::at(const glm::ivec3 &coord) {
  uint32_t hash = hashChunkCoord(coord);
  uint32_t *index = mChunkIndices.get(hash);

  if (index) {
    return mLoadedChunks[*index];
  }
  else {
    return nullptr;
  }
}

const Chunk *Terrain::at(const glm::ivec3 &coord) const {
  uint32_t hash = hashChunkCoord(coord);
  const uint32_t *index = mChunkIndices.get(hash);

  if (index) {
    return mLoadedChunks[*index];
  }
  else {
    return nullptr;
  }
}

glm::ivec3 Terrain::worldToChunkCoord(const glm::vec3 &wPosition) const {
  glm::vec3 scaled = glm::floor(wPosition / (float)CHUNK_DIM);
  return (glm::ivec3)scaled;
}

glm::vec3 Terrain::chunkCoordToWorld(const glm::ivec3 &chunkCoord) const {
  glm::vec3 scaled = glm::floor((glm::vec3)chunkCoord * (float)CHUNK_DIM);
  return scaled;
}

uint32_t Terrain::hashChunkCoord(const glm::ivec3 &coord) const {
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

uint32_t Terrain::getVoxelIndex(int x, int y, int z) const {
  return z * (CHUNK_DIM * CHUNK_DIM) +
    y * CHUNK_DIM + x;
}

uint32_t Terrain::getVoxelIndex(const glm::ivec3 &coord) const {
  return coord.z * (CHUNK_DIM * CHUNK_DIM) +
    coord.y * CHUNK_DIM + coord.x;
}

void Terrain::pushVertexToTriangleList(
  uint32_t v0, uint32_t v1,
  glm::vec3 *vertices, Voxel *voxels,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices, uint32_t &vertexCount) {
  float surfaceLevelF = (float)surfaceDensity.density;
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

  meshVertices[vertexCount].position = vertex;

  ++vertexCount;
}

void Terrain::updateVoxelCube(
  Voxel *voxels,
  const glm::ivec3 &coord,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices,
  uint32_t &vertexCount) {
  uint8_t bitCombination = 0;
  for (uint32_t i = 0; i < 8; ++i) {
    bool isOverSurface = (voxels[i].density > surfaceDensity.density);
    bitCombination |= isOverSurface << i;
  }

  const int8_t *triangleEntry = &VOXEL_EDGE_CONNECT[bitCombination][0];

  uint32_t edge = 0;

  int8_t edgePair[3] = {};

  while(triangleEntry[edge] != -1) {
    int8_t edgeIndex = triangleEntry[edge];
    edgePair[edge % 3] = edgeIndex;

    if (edge % 3 == 2) {
      uint32_t dominantVoxel = 0;

      glm::vec3 vertices[8] = {};
      for (uint32_t i = 0; i < 8; ++i) {
        vertices[i] = NORMALIZED_CUBE_VERTICES[i] +
          glm::vec3(0.5f) + glm::vec3(coord);

        if (voxels[i].density > voxels[dominantVoxel].density) {
          dominantVoxel = i;
        }
      }

      for (uint32_t i = 0; i < 3; ++i) {
        switch(edgePair[i]) {
        case 0: {
          pushVertexToTriangleList(
            0, 1, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 1: {
          pushVertexToTriangleList(
            1, 2, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 2: {
          pushVertexToTriangleList(
            2, 3, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 3: {
          pushVertexToTriangleList(
            3, 0, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 4: {
          pushVertexToTriangleList(
            4, 5, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 5: {
          pushVertexToTriangleList(
            5, 6, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 6: {
          pushVertexToTriangleList(
            6, 7, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 7: {
          pushVertexToTriangleList(
            7, 4, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 8: {
          pushVertexToTriangleList(
            0, 4, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 9: {
          pushVertexToTriangleList(
            1, 5, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 10: {
          pushVertexToTriangleList(
            2, 6, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        case 11: {
          pushVertexToTriangleList(
            3, 7, vertices, voxels, surfaceDensity,
            meshVertices, vertexCount);
        } break;

        }
      }
    }

    ++edge;
  }
}

uint32_t Terrain::generateChunkVertices(
  const Chunk &chunk,
  Voxel surfaceDensity,
  ChunkVertex *meshVertices) {
  uint32_t vertexCount = 0;

  const Chunk *xSuperior = at(chunk.chunkCoord + glm::ivec3(1, 0, 0));
  const Chunk *ySuperior = at(chunk.chunkCoord + glm::ivec3(0, 1, 0));
  const Chunk *zSuperior = at(chunk.chunkCoord + glm::ivec3(0, 0, 1));
    
  bool doesntExist = 0;
  if (xSuperior) {
    // x_superior
    for (uint32_t z = 0; z < CHUNK_DIM; ++z) {
      for (uint32_t y = 0; y < CHUNK_DIM - 1; ++y) {
        doesntExist = 0;
                
        uint32_t x = CHUNK_DIM - 1;

        Voxel voxelValues[8] = {
          chunk.voxels[getVoxelIndex(x, y, z)],
          getChunkEdgeVoxel(x + 1, y, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y, z + 1, &doesntExist, chunk.chunkCoord),
                    
          chunk.voxels[getVoxelIndex(x, y + 1, z)],
          getChunkEdgeVoxel(x + 1, y + 1, z,&doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y + 1, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y + 1, z + 1, &doesntExist, chunk.chunkCoord)
        };

        if (!doesntExist) {
          updateVoxelCube(
            voxelValues, glm::ivec3(x, y, z), surfaceDensity,
            meshVertices, vertexCount);
        }
      }
    }
  }

  if (ySuperior) {
    // y_superior    
    for (uint32_t z = 0; z < CHUNK_DIM; ++z) {
      for (uint32_t x = 0; x < CHUNK_DIM; ++x) {
        doesntExist = 0;
                
        uint32_t y = CHUNK_DIM - 1;

        Voxel voxelValues[8] = {
          chunk.voxels[getVoxelIndex(x, y, z)],
          getChunkEdgeVoxel(x + 1, y, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y, z + 1, &doesntExist, chunk.chunkCoord),
                    
          getChunkEdgeVoxel(x, y + 1, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y + 1, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y + 1, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y + 1, z + 1, &doesntExist, chunk.chunkCoord)
        };

        if (!doesntExist) {
          updateVoxelCube(
            voxelValues, glm::ivec3(x, y, z), surfaceDensity,
            meshVertices, vertexCount);
        }
      }
    }
  }

  if (zSuperior) {
    // z_superior
    for (uint32_t y = 0; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 0; x < CHUNK_DIM - 1; ++x) {
        doesntExist = 0;
                
        uint32_t z = CHUNK_DIM - 1;

        Voxel voxelValues[8] = {
          chunk.voxels[getVoxelIndex(x, y, z)],
          getChunkEdgeVoxel(x + 1, y, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y, z + 1, &doesntExist, chunk.chunkCoord),
                    
          chunk.voxels[getVoxelIndex(x, y + 1, z)],
          getChunkEdgeVoxel(x + 1, y + 1, z, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x + 1, y + 1, z + 1, &doesntExist, chunk.chunkCoord),
          getChunkEdgeVoxel(x,     y + 1, z + 1, &doesntExist, chunk.chunkCoord)
        };

        if (!doesntExist) {
          updateVoxelCube(
            voxelValues, glm::ivec3(x, y, z), surfaceDensity,
            meshVertices, vertexCount);
        }
      }
    }
  }
    
  for (uint32_t z = 0; z < CHUNK_DIM - 1; ++z) {
    for (uint32_t y = 0; y < CHUNK_DIM - 1; ++y) {
      for (uint32_t x = 0; x < CHUNK_DIM - 1; ++x) {
        Voxel voxelValues[8] = {
          chunk.voxels[getVoxelIndex(x, y, z)],
          chunk.voxels[getVoxelIndex(x + 1, y, z)],
          chunk.voxels[getVoxelIndex(x + 1, y, z + 1)],
          chunk.voxels[getVoxelIndex(x, y, z + 1)],
                    
          chunk.voxels[getVoxelIndex(x, y + 1, z)],
          chunk.voxels[getVoxelIndex(x + 1, y + 1, z)],
          chunk.voxels[getVoxelIndex(x + 1, y + 1, z + 1)],
          chunk.voxels[getVoxelIndex(x, y + 1, z + 1)]
        };

        updateVoxelCube(
          voxelValues, glm::ivec3(x, y, z), surfaceDensity,
          meshVertices, vertexCount);
      }
    }
  }

  return vertexCount;
}

Voxel Terrain::getChunkEdgeVoxel(
  int x, int y, int z,
  bool *doesntExist,
  const glm::ivec3 &chunkCoord) const {
  glm::ivec3 chunkCoordOffset = glm::ivec3(0);
  glm::ivec3 finalCoord = glm::ivec3(x, y, z);

  if (x == CHUNK_DIM) {
    finalCoord.x = 0;
    chunkCoordOffset.x = 1;
  }
  if (y == CHUNK_DIM) {
    finalCoord.y = 0;
    chunkCoordOffset.y = 1;
  }
  if (z == CHUNK_DIM) {
    finalCoord.z = 0;
    chunkCoordOffset.z = 1;
  }

  glm::ivec3 newChunkCoord = glm::ivec3(
    chunkCoord.x + chunkCoordOffset.x,
    chunkCoord.y + chunkCoordOffset.y,
    chunkCoord.z + chunkCoordOffset.z);

  const Chunk *chunk = at(newChunkCoord);
    
  *doesntExist = (bool)(chunk == nullptr);

  if (*doesntExist)
    return { 0 };
    
  return chunk->voxels[getVoxelIndex(finalCoord.x, finalCoord.y, finalCoord.z)];
}

ChunkVertices Terrain::createChunkVertices(
  const Chunk &chunk, VulkanContext &graphicsContext) {
  Voxel surfaceDensity = { 1000 };
  uint32_t vertexCount = generateChunkVertices(
    chunk, surfaceDensity, mTemporaryVertices);

  ChunkVertices ret = {};
  ret.vbo.init(
    graphicsContext.device(),
    vertexCount * sizeof(ChunkVertex),
    (VulkanBufferFlagBits)VulkanBufferFlag::VertexBuffer);

  ret.vbo.fillWithStaging(
    graphicsContext.device(),
    graphicsContext.commandPool(),
    {(uint8_t *)mTemporaryVertices, vertexCount});

  ret.vertexCount = vertexCount;

  return ret;
}

void Terrain::makeSphere(float radius, const glm::vec3 &center) {
  glm::ivec3 start = (glm::ivec3)center - glm::ivec3((int32_t)radius);
  float radius2 = radius * radius;
  int32_t diameter = (int32_t)radius * 2 + 1;

  glm::ivec3 currentChunkCoord = worldToChunkCoord(center);

  Chunk *currentChunk = getChunk(currentChunkCoord);

  for (int32_t z = start.z; z < start.z + diameter; ++z) {
    for (int32_t y = start.y; y < start.y + diameter; ++y) {
      for (int32_t x = start.x; x < start.x + diameter; ++x) {
        glm::ivec3 position = glm::ivec3(x, y, z);
        glm::vec3 posFloat = (glm::vec3)position;
        glm::vec3 diff = posFloat - center;

        float distance2 = glm::dot(diff, diff);

        if (distance2 <= radius2) {
          glm::ivec3 c = worldToChunkCoord(posFloat);

          glm::ivec3 chunkOriginDiff = position - c * (int32_t)CHUNK_DIM;

          if (chunkOriginDiff.x >= 0 && chunkOriginDiff.x < 16 &&
              chunkOriginDiff.y >= 0 && chunkOriginDiff.y < 16 &&
              chunkOriginDiff.z >= 0 && chunkOriginDiff.z < 16) {
            // Is within current chunk boundaries
            float proportion = 1.0f - (distance2 / radius2);

            glm::ivec3 voxelCoord = chunkOriginDiff;

            Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
            uint16_t newValue = (uint32_t)((proportion) * 2000.0f);
            v->density = newValue;
            LOG_INFOV(
              "Voxel at %s has density %d\n",
              glm::to_string(position).c_str(), (int)newValue);
          }
          else {
            glm::ivec3 c = worldToChunkCoord(position);

            currentChunk = getChunk(c);
            currentChunkCoord = c;

            float proportion = 1.0f / (distance2 / radius2);

            glm::ivec3 voxelCoord = position -
              currentChunkCoord * (int32_t)CHUNK_DIM;

            Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
            uint16_t newValue = (uint32_t)((proportion) * 2000.0f);
            v->density = newValue;
            LOG_INFOV(
              "Voxel at %s has density %d\n",
              glm::to_string(position).c_str(), (int)newValue);
          }
        }
      }
    }
  }
}

void Terrain::prepareForRender(VulkanContext &graphicsContext) {
  for (int i = 0; i < mLoadedChunks.size; ++i) {
    Chunk *chunk = mLoadedChunks[i];
    // Don't worry, this will be thoroughly redone
    chunk->vertices = createChunkVertices(*chunk, graphicsContext);
  }
}

void Terrain::submitForRender(
  const Camera &camera,
  const PlanetRenderer &planet,
  const Clipping &clipping,
  VulkanFrame &frame) {
  
}

}
