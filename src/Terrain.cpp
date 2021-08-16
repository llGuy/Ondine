#include "Math.hpp"
#include "Camera.hpp"
#include "Terrain.hpp"
#include "Clipping.hpp"
#include <glm/gtc/noise.hpp>
#include "VulkanContext.hpp"
#include "PlanetRenderer.hpp"
#include <glm/gtx/string_cast.hpp>

namespace Ondine::Graphics {

void Terrain::init() {
  mTerrainScale = 40;
  mChunkWidth = mTerrainScale * (float)CHUNK_DIM;

  mChunkIndices.init();
  mFlatChunkIndices.init();
  mLoadedChunks.init(MAX_CHUNKS);
  mUpdatedChunks.init(MAX_CHUNKS);
  mNullChunk = flAlloc<Chunk>();
  memset(mNullChunk, 0, sizeof(Chunk));

  mMaxVoxelDensity = (float)0xFFFF;
  mUpdated = false;
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
    addToFlatChunkIndices(chunk);

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
  glm::vec3 scaled = glm::floor(wPosition / ((float)CHUNK_DIM));
  return (glm::ivec3)scaled;
}

glm::vec3 Terrain::chunkCoordToWorld(const glm::ivec3 &chunkCoord) const {
  glm::vec3 scaled = glm::floor(
    (glm::vec3)chunkCoord * (float)(CHUNK_DIM));
  return scaled;
}

void Terrain::makeSphere(float radius, glm::vec3 center, float intensity) {
  radius /= mTerrainScale;
  center /= mTerrainScale;

  glm::ivec3 start = (glm::ivec3)center - glm::ivec3((int32_t)radius);
  float radius2 = radius * radius;
  int32_t diameter = (int32_t)radius * 2 + 1;

  glm::ivec3 currentChunkCoord = worldToChunkCoord(center);
  Chunk *currentChunk = getChunk(currentChunkCoord);
  markChunkForUpdate(currentChunk);

  for (int32_t z = start.z; z < start.z + diameter; ++z) {
    for (int32_t y = start.y; y < start.y + diameter; ++y) {
      for (int32_t x = start.x; x < start.x + diameter; ++x) {
        glm::ivec3 position = glm::ivec3(x, y, z);
        glm::vec3 posFloat = (glm::vec3)position;
        glm::vec3 diff = posFloat - center;

        float distance2 = glm::dot(diff, diff);

        if (distance2 <= radius2) {
          glm::ivec3 chunkOriginDiff = position -
            currentChunkCoord * (int32_t)CHUNK_DIM;

          if (chunkOriginDiff.x >= 0 && chunkOriginDiff.x < CHUNK_DIM &&
              chunkOriginDiff.y >= 0 && chunkOriginDiff.y < CHUNK_DIM &&
              chunkOriginDiff.z >= 0 && chunkOriginDiff.z < CHUNK_DIM) {
            // Is within current chunk boundaries
            float proportion = (1.0f - (distance2 / radius2)) * intensity;

            glm::ivec3 voxelCoord = chunkOriginDiff;

            Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
            uint16_t addedValue = (uint32_t)((proportion) * mMaxVoxelDensity);

            uint32_t finalValue = (uint32_t)addedValue + (uint32_t)v->density;
            finalValue = glm::min(finalValue, MAX_DENSITY);
            
            v->density = finalValue;
          }
          else {
            glm::ivec3 c = worldToChunkCoord(position);

            currentChunk = getChunk(c);
            currentChunkCoord = c;
            markChunkForUpdate(currentChunk);

            float proportion = (1.0f - (distance2 / radius2)) * intensity;

            glm::ivec3 voxelCoord = position -
              currentChunkCoord * (int32_t)CHUNK_DIM;

            Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
            uint16_t addedValue = (uint32_t)((proportion) * mMaxVoxelDensity);
            uint32_t finalValue = (uint32_t)addedValue + (uint32_t)v->density;
            finalValue = glm::min(finalValue, MAX_DENSITY);
            v->density = finalValue;
          }
        }
      }
    }
  }
}

void Terrain::makePlane(float radius, glm::vec3 center, float intensity) {
  radius /= mTerrainScale;
  center /= mTerrainScale;

  glm::ivec3 start = (glm::ivec3)center - glm::ivec3((int32_t)radius);
  float radius2 = radius * radius;
  int32_t diameter = (int32_t)radius * 2 + 1;

  glm::ivec3 currentChunkCoord = worldToChunkCoord(center);
  Chunk *currentChunk = getChunk(currentChunkCoord);
  markChunkForUpdate(currentChunk);

  int32_t y = (int)center.y;

  for (int32_t z = start.z; z < start.z + diameter; ++z) {
    for (int32_t x = start.x; x < start.x + diameter; ++x) {
      glm::ivec3 position = glm::ivec3(x, y, z);
      glm::vec3 posFloat = (glm::vec3)position;
      glm::vec3 diff = posFloat - center;

      float distance2 = glm::dot(diff, diff);

      if (distance2 <= radius2) {
        glm::ivec3 chunkOriginDiff = position -
          currentChunkCoord * (int32_t)CHUNK_DIM;

        if (chunkOriginDiff.x >= 0 && chunkOriginDiff.x < CHUNK_DIM &&
            chunkOriginDiff.y >= 0 && chunkOriginDiff.y < CHUNK_DIM &&
            chunkOriginDiff.z >= 0 && chunkOriginDiff.z < CHUNK_DIM) {
          // Is within current chunk boundaries
          float proportion = (1.0f - (distance2 / radius2)) * intensity;

          glm::ivec3 voxelCoord = chunkOriginDiff;

          Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
          uint16_t addedValue = (uint32_t)((proportion) * mMaxVoxelDensity);

          uint32_t finalValue = (uint32_t)addedValue + (uint32_t)v->density;
          finalValue = glm::min(finalValue, MAX_DENSITY);
            
          v->density = finalValue;
        }
        else {
          glm::ivec3 c = worldToChunkCoord(position);

          currentChunk = getChunk(c);
          currentChunkCoord = c;
          markChunkForUpdate(currentChunk);

          float proportion = (1.0f - (distance2 / radius2)) * intensity;

          glm::ivec3 voxelCoord = position -
            currentChunkCoord * (int32_t)CHUNK_DIM;

          Voxel *v = &currentChunk->voxels[getVoxelIndex(voxelCoord)];
          uint16_t addedValue = (uint32_t)((proportion) * mMaxVoxelDensity);
          uint32_t finalValue = (uint32_t)addedValue + (uint32_t)v->density;
          finalValue = glm::min(finalValue, MAX_DENSITY);
          v->density = finalValue;
        }
      }
    }
  }
}

void Terrain::makeIslands(
  float seaLevel, uint32_t octaveCount,
  float persistance, float lacunarity,
  float baseAmplitude, float baseFrequency,
  glm::vec2 s, glm::vec2 e) {
  seaLevel /= mTerrainScale;
  s /= mTerrainScale;
  e /= mTerrainScale;

  glm::ivec2 start = (glm::ivec2)s;
  glm::ivec2 end = (glm::ivec2)e;

  glm::vec2 range = (glm::vec2)(end - start);

  glm::ivec3 currentChunkCoord = worldToChunkCoord(
    glm::vec3(start.x, seaLevel, start.y));
  Chunk *currentChunk = getChunk(currentChunkCoord);
  markChunkForUpdate(currentChunk);

  int32_t minHeight = (int32_t)seaLevel - 5;

  for (int32_t z = start.y; z < end.y; ++z) {
    for (int32_t x = start.x; x < end.x; ++x) {
      float height = seaLevel;
      float freq = baseFrequency;
      float amp = baseAmplitude;

      for (int i = 0; i < octaveCount; ++i) {
        glm::vec2 perlinCoord = glm::vec2(
          float(x - start.x) / (float)range.x,
          float(z - start.y) / (float)range.y) * freq;

        float noise = glm::perlin(perlinCoord);
        height += (noise * amp);

        amp += persistance;
        freq *= lacunarity;
      }

      height = fmax(height, minHeight);

      for (int32_t y = (int32_t)minHeight; y < (int32_t)height; ++y) {
        glm::ivec3 position = glm::ivec3(x, y, z);
        glm::ivec3 chunkOriginDiff = position -
          currentChunkCoord * (int32_t)CHUNK_DIM;

        float proportion = 1.0f - (y - minHeight) / (height - minHeight);
        proportion *= 0.8f;

        if (chunkOriginDiff.x >= 0 && chunkOriginDiff.x < CHUNK_DIM &&
            chunkOriginDiff.y >= 0 && chunkOriginDiff.y < CHUNK_DIM &&
            chunkOriginDiff.z >= 0 && chunkOriginDiff.z < CHUNK_DIM) {
          currentChunk->voxels[getVoxelIndex(chunkOriginDiff)].density = 
            (uint16_t)(mMaxVoxelDensity) * proportion;
        }
        else {
          glm::ivec3 c = worldToChunkCoord(position);

          currentChunk = getChunk(c);
          currentChunkCoord = c;

          markChunkForUpdate(currentChunk);

          chunkOriginDiff = position -
            currentChunkCoord * (int32_t)CHUNK_DIM;

          currentChunk->voxels[getVoxelIndex(chunkOriginDiff)].density = 
            (uint16_t)(mMaxVoxelDensity) * proportion;
        }
      }
    }
  }

  mUpdated = true;
}

void Terrain::paint(
  glm::vec3 position,
  glm::vec3 direction,
  float radius,
  float strength) {
  position /= (float)mTerrainScale;

  glm::vec3 step = glm::normalize(direction) * 2.0f;

  const uint32_t MAX_STEP_COUNT = 50;

  bool outside = true;

  for (int i = 0; i < MAX_STEP_COUNT; ++i) {
    position += step;
    glm::ivec3 chunkCoord = worldToChunkCoord(position);
    Chunk *c = at(chunkCoord);

    if (c) {
      glm::ivec3 chunkOriginDiff = (glm::ivec3)position -
        c->chunkCoord * (int32_t)CHUNK_DIM;
      uint32_t voxelIndex = getVoxelIndex(chunkOriginDiff);

      if (c->voxels[voxelIndex].density > SURFACE_DENSITY && outside) {
        step /= -2.0f;
        outside = false;
      }
      else if (c->voxels[voxelIndex].density < SURFACE_DENSITY && !outside) {
        step /= -2.0f;
        outside = true;
      }
    }
  }

  if (!outside) {
    makeSphere(radius, position * (float)mTerrainScale, strength);
  }
}

void Terrain::generateChunkFaceNormals(
  Chunk *chunk,
  Chunk *p, Chunk *n,
  uint32_t dimension) {
  auto density = [chunk, this](const glm::ivec3 &coord) {
    return chunk->voxels[getVoxelIndex(coord)].density;
  };

  glm::ivec3 diff[3] = {
    glm::ivec3(1, 0, 0),
    glm::ivec3(0, 1, 0),
    glm::ivec3(0, 0, 1)
  };

  apply2D(
    1, CHUNK_DIM - 1,
    [this, chunk, density, diff, p, n, dimension](
      const glm::ivec2 &voxelCoordXY) {
      // One pass for when z == CHUNK_DIM, one for when z == 0
      glm::ivec3 voxelCoord;
      glm::ivec3 converted;
      glm::vec3 grad;

      for (int i = 0, j = 0; i < 3; ++i) {
        if (i == dimension) {
          voxelCoord[i] = CHUNK_DIM - 1;
          converted[i] = 0;
        }
        else {
          voxelCoord[i] = voxelCoordXY[j++];
          converted[i] = voxelCoord[i];
        }
      }

      for (int i = 0; i < 3; ++i) {
        if (i == dimension)
          grad[i] = p->voxels[getVoxelIndex(converted)].density -
            density(voxelCoord - diff[i]);
        else
          grad[i] = density(voxelCoord + diff[i]) -
            density(voxelCoord - diff[i]);
      }

      setVoxelNormal(chunk, voxelCoord, grad);

      for (int i = 0, j = 0; i < 3; ++i) {
        if (i == dimension) {
          voxelCoord[i] = 0;
          converted[i] = CHUNK_DIM - 1;
        }
        else {
          voxelCoord[i] = voxelCoordXY[j++];
          converted[i] = voxelCoord[i];
        }
      }

      for (int i = 0; i < 3; ++i) {
        if (i == dimension) 
          grad[i] = density(voxelCoord + diff[i]) - 
            n->voxels[getVoxelIndex(converted)].density;
        else
          grad[i] = density(voxelCoord + diff[i]) -
            density(voxelCoord - diff[i]);
      }

      setVoxelNormal(chunk, voxelCoord, grad);
    });
}

void Terrain::setVoxelNormal(
  Chunk *chunk,
  const glm::ivec3 &voxelCoord,
  const glm::vec3 &grad) {
  glm::vec3 normal = -glm::normalize(grad);
  chunk->voxels[getVoxelIndex(voxelCoord)].normalX =
    (int)(normal.x * 1000.0f);
  chunk->voxels[getVoxelIndex(voxelCoord)].normalY =
    (int)(normal.y * 1000.0f);
  chunk->voxels[getVoxelIndex(voxelCoord)].normalZ =
    (int)(normal.z * 1000.0f);
}

void Terrain::generateVoxelNormals() {
  for (int i = 0; i < mUpdatedChunks.size; ++i) {
    uint32_t chunkIndex = mUpdatedChunks[i];
    Chunk *chunk = mLoadedChunks[chunkIndex];

    auto density = [chunk, this](const glm::ivec3 &coord) {
      return chunk->voxels[getVoxelIndex(coord)].density;
    };

    glm::ivec3 diff[3] = {
      glm::ivec3(1, 0, 0),
      glm::ivec3(0, 1, 0),
      glm::ivec3(0, 0, 1)
    };

    apply3D(
      1, CHUNK_DIM - 1,
      [this, chunk, density, diff](const glm::ivec3 &voxelCoord) {
        glm::vec3 grad;
        grad.x = density(voxelCoord + diff[0]) - density(voxelCoord - diff[0]);
        grad.y = density(voxelCoord + diff[1]) - density(voxelCoord - diff[1]);
        grad.z = density(voxelCoord + diff[2]) - density(voxelCoord - diff[2]);
        setVoxelNormal(chunk, voxelCoord, grad);
      });

    Chunk *px = at(chunk->chunkCoord + glm::ivec3(1, 0, 0));
    if (!px) px = mNullChunk;
    Chunk *nx = at(chunk->chunkCoord - glm::ivec3(1, 0, 0));
    if (!nx) nx = mNullChunk;
    Chunk *py = at(chunk->chunkCoord + glm::ivec3(0, 1, 0));
    if (!py) py = mNullChunk;
    Chunk *ny = at(chunk->chunkCoord - glm::ivec3(0, 1, 0));
    if (!ny) ny = mNullChunk;
    Chunk *pz = at(chunk->chunkCoord + glm::ivec3(0, 0, 1));
    if (!pz) pz = mNullChunk;
    Chunk *nz = at(chunk->chunkCoord - glm::ivec3(0, 0, 1));
    if (!nz) nz = mNullChunk;

    generateChunkFaceNormals(chunk, px, nx, 0);
    generateChunkFaceNormals(chunk, py, ny, 1);
    generateChunkFaceNormals(chunk, pz, nz, 2);

    auto densitym = [chunk, this](const glm::ivec3 &coord) {
      glm::ivec3 chunkCoordOffset = glm::ivec3(0);
      glm::ivec3 voxelCoord = coord;
      for (int i = 0; i < 3; ++i) {
        if (coord[i] > (int)CHUNK_DIM - 1) {
          chunkCoordOffset[i] = 1;
          voxelCoord[i] = 0;
        }
        else if (coord[i] < 0) {
          chunkCoordOffset[i] = -1; 
          voxelCoord[i] = (int)CHUNK_DIM - 1;
        }
      }

      Chunk *actualChunk = at(chunk->chunkCoord + chunkCoordOffset);
      if (!actualChunk) actualChunk = mNullChunk;

      return actualChunk->voxels[getVoxelIndex(voxelCoord)].density;
    };

    glm::ivec3 corners[] = {
      glm::ivec3(0, 0, 0) * int(CHUNK_DIM - 1),
      glm::ivec3(0, 0, 1) * int(CHUNK_DIM - 1),
      glm::ivec3(0, 1, 0) * int(CHUNK_DIM - 1),
      glm::ivec3(0, 1, 1) * int(CHUNK_DIM - 1),

      glm::ivec3(1, 0, 0) * int(CHUNK_DIM - 1),
      glm::ivec3(1, 0, 1) * int(CHUNK_DIM - 1),
      glm::ivec3(1, 1, 0) * int(CHUNK_DIM - 1),
      glm::ivec3(1, 1, 1) * int(CHUNK_DIM - 1),
    };

    for (int i = 0; i < 8; ++i) {
      const glm::ivec3 &voxelCoord = corners[i];
      glm::vec3 grad;
      grad.x = densitym(voxelCoord + diff[0]) - densitym(voxelCoord - diff[0]);
      grad.y = densitym(voxelCoord + diff[1]) - densitym(voxelCoord - diff[1]);
      grad.z = densitym(voxelCoord + diff[2]) - densitym(voxelCoord - diff[2]);
      setVoxelNormal(chunk, voxelCoord, grad);
    }

    // Edges
    for (int dim = 0; dim < 3; ++dim) {
      const glm::ivec2 PERMUTATIONS_2D[4] = {
        glm::ivec2(0, 0),
        glm::ivec2(0, 1),
        glm::ivec2(1, 0),
        glm::ivec2(1, 1)
      };

      glm::ivec3 permutations[4];
      for (int p = 0; p < 4; ++p) {
        permutations[p][dim] = 0;

        for (int i = 1; i < 3; ++i) {
          permutations[p][(dim + i) % 3] =
            PERMUTATIONS_2D[p][i - 1] * (CHUNK_DIM - 1);
        }

        for (int component = 1; component < CHUNK_DIM - 1; ++component) {
          glm::ivec3 voxelCoord = permutations[p];
          voxelCoord[dim] = component;

          glm::vec3 grad;
          grad.x = densitym(voxelCoord + diff[0])-densitym(voxelCoord - diff[0]);
          grad.y = densitym(voxelCoord + diff[1])-densitym(voxelCoord - diff[1]);
          grad.z = densitym(voxelCoord + diff[2])-densitym(voxelCoord - diff[2]);
          setVoxelNormal(chunk, voxelCoord, grad);
        }
      }
    }
  }
}

void Terrain::markChunkForUpdate(Chunk *chunk) {
  if (!chunk->needsUpdating) {
    chunk->needsUpdating = true;
    mUpdatedChunks[mUpdatedChunks.size++] = *mChunkIndices.get(
      hashChunkCoord(chunk->chunkCoord));
  }
}

void Terrain::addToFlatChunkIndices(Chunk *chunk) {
  int x = chunk->chunkCoord.x, z = chunk->chunkCoord.z;
  uint32_t hash = hashFlatChunkCoord(glm::ivec2(x, z));
  uint32_t *index = mFlatChunkIndices.get(hash);

  if (index) {
    Chunk *head = mLoadedChunks[*index];
    chunk->next = head->chunkStackIndex;
    *index = chunk->chunkStackIndex;
  }
  else {
    mFlatChunkIndices.insert(hash, chunk->chunkStackIndex);
    chunk->next = INVALID_CHUNK_INDEX;
  }
}

Chunk *Terrain::getFirstFlatChunk(glm::ivec2 flatCoord) {
  uint32_t hash = hashFlatChunkCoord(flatCoord);
  uint32_t *index = mFlatChunkIndices.get(hash);

  if (index) {
    return mLoadedChunks[*index];
  }
  else {
    return nullptr;
  }
}

const Voxel &Terrain::getVoxel(const glm::vec3 &position) const {
  glm::ivec3 chunkCoord = worldToChunkCoord(position);
  glm::ivec3 iposition = (glm::ivec3)position;

  iposition -= chunkCoord * (int)CHUNK_DIM;

  const Chunk *chunk = at(chunkCoord);

  if (chunk) {
    return chunk->voxels[getVoxelIndex(iposition)];
  }
  else {
    static Voxel nullVoxel = {};
    return nullVoxel;
  }
}

}
