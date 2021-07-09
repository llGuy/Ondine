#include "Math.hpp"
#include "Terrain.hpp"

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

uint32_t Terrain::getVoxelIndex(const glm::ivec3 &coord) const {
  return coord.z * (CHUNK_DIM * CHUNK_DIM) +
    coord.y * CHUNK_DIM + coord.x;
}

void Terrain::pushVertextoTriangleList(
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

ChunkVertices Terrain::generateChunkVertices(
  const Chunk &chunk) {
  return {};
}

Voxel Terrain::getChunkEdgeVoxel(
  const glm::ivec3 &inCoord,
  bool *doesntExist,
  const glm::ivec3 &chunkCoord) const {
  glm::ivec3 chunkCoordOffset = glm::ivec3(0);
  glm::ivec3 finalCoord = inCoord;

  if (inCoord.x == CHUNK_DIM) {
    finalCoord.x = 0;
    chunkCoordOffset.x = 1;
  }
  if (inCoord.y == CHUNK_DIM) {
    finalCoord.y = 0;
    chunkCoordOffset.y = 1;
  }
  if (inCoord.z == CHUNK_DIM) {
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
    
  return chunk->voxels[getVoxelIndex(finalCoord)];
}

}
