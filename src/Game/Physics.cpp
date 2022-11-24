#include "Math.hpp"
#include "Utils.hpp"
#include "Entity.hpp"
#include "Memory.hpp"
#include "Physics.hpp"
#include "Geometry.hpp"

// http://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf

namespace Ondine::Game::Physics {

static bool simplexLine(Simplex &simplex, glm::vec3 &d) {
  glm::vec3 a = simplex[0];
  glm::vec3 b = simplex[1];

  glm::vec3 ab = b - a;
  glm::vec3 a0 =   - a;

  if (isSameDirection(ab, a0)) {
    d = glm::cross(glm::cross(ab, a0), ab);
  }
  else {
    simplex = createSimplex(a);
    d = a0;
  }

  return false;
}

bool simplexTriangle(Simplex &simplex, glm::vec3 &d) {
  glm::vec3 a = simplex[0];
  glm::vec3 b = simplex[1];
  glm::vec3 c = simplex[2];

  glm::vec3 ab = b - a;
  glm::vec3 ac = c - a;
  glm::vec3 a0 =   - a;

  glm::vec3 abc = glm::cross(ab, ac);

  if (isSameDirection(glm::cross(abc, ac), a0)) {
    if (isSameDirection(ac, a0)) {
      simplex = createSimplex(a, c);
      d = glm::cross(glm::cross(ac, a0), ac);
    }
    else {
      return simplexLine(simplex = createSimplex(a, b), d);
    }
  }
  else {
    if (isSameDirection(glm::cross(ab, abc), a0)) {
      return simplexLine(simplex = createSimplex(a, b), d);
    }
    else {
      if (isSameDirection(abc, a0)) {
        d = abc;
      }
      else {
        simplex = createSimplex(a, c, b);
        d = -abc;
      }
    }
  }

  return false;
}

bool simplexTetrahedron(Simplex &simplex, glm::vec3 &dir) {
  glm::vec3 a = simplex[0];
  glm::vec3 b = simplex[1];
  glm::vec3 c = simplex[2];
  glm::vec3 d = simplex[3];

  glm::vec3 ab = b - a;
  glm::vec3 ac = c - a;
  glm::vec3 ad = d - a;
  glm::vec3 a0 =   - a;

  glm::vec3 abc = glm::cross(ab, ac);
  glm::vec3 acd = glm::cross(ac, ad);
  glm::vec3 adb = glm::cross(ad, ab);

  if (isSameDirection(abc, a0)) {
    return simplexTriangle(simplex = createSimplex(a, b, c), dir);
  }
  
  if (isSameDirection(acd, a0)) {
    return simplexTriangle(simplex = createSimplex(a, c, d), dir);
  }

  if (isSameDirection(adb, a0)) {
    return simplexTriangle(simplex = createSimplex(a, d, b), dir);
  }

  return true;
}

bool nextSimplex(Simplex &simplex, glm::vec3 &d) {
  switch (simplex.size) {
    case 2: return simplexLine(simplex, d);
    case 3: return simplexTriangle(simplex, d);
    case 4: return simplexTetrahedron(simplex, d);
  }

  PANIC_AND_EXIT();

  return false;
}

CollisionMesh createCollisionMesh(const Graphics::Geometry &geometry, const Entity &entity) {
  auto [vertices, vertexCount] = geometry.getVertices();

  CollisionMesh mesh = {};
  mesh.vertexCount = vertexCount;
  mesh.vertices = lnAllocv<glm::vec3>(mesh.vertexCount);

  for (int i = 0; i < mesh.vertexCount; ++i) {
    glm::vec3 transformedVert = entity.position + 
      glm::mat3_cast(entity.rotation) * 
      (vertices[i] * entity.scale);

    mesh.vertices[i] = transformedVert;
  }

  return mesh;
}

uint32_t findFurthestPointIdx(const CollisionMesh &m, const glm::vec3 &d) {
  float maxDistance = glm::dot(d, m.vertices[0]);
  uint32_t vertexIdx = 0;

  for (int i = 1; i < m.vertexCount; ++i) {
    float dp = glm::dot(d, m.vertices[i]);
    if (dp > maxDistance) {
      maxDistance = dp;
      vertexIdx = i;
    }
  }

  assert(vertexIdx != kMaxUint32);

  return vertexIdx;
}

glm::vec3 findFurthestPoint(const CollisionMesh &m, const glm::vec3 &d) {
  return m.vertices[findFurthestPointIdx(m, d)];
}

glm::vec3 findSupportPointMinkowski(
  const CollisionMesh &a,
  const CollisionMesh &b,
  const glm::vec3 &d) {
  return findFurthestPoint(a, d) - findFurthestPoint(b, -d);
}

// Assume that minkowskiPoint is normalized
uint32_t findOriginalVertex(
  const glm::vec3 &minkowskiPoint,
  const CollisionMesh &a) {

  return findFurthestPointIdx(a, minkowskiPoint);
}

GJK gjkOverlap(const CollisionMesh &a, const CollisionMesh &b) {
  glm::vec3 support = findSupportPointMinkowski(a, b, glm::vec3(1.0f, 0.0f, 0.0f));

  Simplex simplex;
  simplex.add(support);

  // Look towards the origin
  glm::vec3 d = -support;

  for (;;) {
    support = findSupportPointMinkowski(a, b, d);

    if (glm::dot(support, d) <= 0.0f) {
      // This means that there wasn't anything on the other size of the origin
      // There was no collision - the origin must be contained in minkowski difference
      return {false, simplex};
    }

    simplex.add(support);

    if (nextSimplex(simplex, d)) {
      return {true, simplex};
    }
  }

  return {};
}

struct EPAFaceNormal {
  glm::vec3 n;
  float dist;
};

struct EPAFaceNormals {
  std::vector<EPAFaceNormal> normals;
  uint32_t minNormalIdx;
  glm::ivec3 minVertexIndices;
};

EPAFaceNormals getFaceNormals(
  const std::vector<glm::vec3> &polytope,
  const std::vector<uint32_t> &faces) {
  EPAFaceNormals ret = {};
  uint32_t minIdx = 0;
  glm::ivec3 minVertexIndices;
  float minDist = FLT_MAX;

  for (uint32_t i = 0; i < faces.size(); i += 3) {
    glm::vec3 a = polytope[faces[i]];
    glm::vec3 b = polytope[faces[i+1]];
    glm::vec3 c = polytope[faces[i+2]];

    glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
    float dist = glm::dot(normal, a);

    // Just in case the orientation of the vertices leads to negative normal
    if (dist < 0) {
      normal *= -1.0f;
      dist *= -1.0f;
    }

    ret.normals.push_back({ normal, dist });

    if (dist < minDist) {
      minIdx = i / 3;
      minDist = dist;
      minVertexIndices = glm::ivec3(faces[i], faces[i+1], faces[i+2]);
    }
  }

  ret.minNormalIdx = minIdx;
  ret.minVertexIndices = minVertexIndices;

  return ret;
}

static void addUniqueEdge(
  std::vector<std::pair<uint32_t, uint32_t>> &edges,
  const std::vector<uint32_t> &faces,
  uint32_t a,
  uint32_t b) {
  auto reverse = std::find(edges.begin(), edges.end(), std::make_pair(faces[b], faces[a]));

  if (reverse != edges.end()) {
    edges.erase(reverse);
  }
  else {
    edges.emplace_back(faces[a], faces[b]);
  }
}

// Project origin to plane containing triangle ABC
static glm::vec3 getContactPointBarycentricCoords(
  const glm::vec3 &a,
  const glm::vec3 &b,
  const glm::vec3 &c) {
  glm::vec3 normal = glm::normalize(glm::cross(b-a, c-a));
  // Distance from origin to triangle
  float dist = glm::dot(normal, a);
  glm::vec3 p = normal * dist;

  // This is the point in Minkowski space where the collision happened
  return getBarycentricCoordinates(p, a, b, c);
}

static Collision epaGetCollision(
  const Simplex &simplex,
  const CollisionMesh &a,
  const CollisionMesh &b) {
  std::vector<glm::vec3> polytope(simplex.vertices, simplex.vertices + simplex.size);
  std::vector<uint32_t> faces = { 
    0, 1, 2,
    0, 3, 1,
    0, 2, 3,
    1, 3, 2
  };

  auto [normals, minNormalIdx, minVertexIndices] = getFaceNormals(polytope, faces);

  glm::vec3 minNormal;
  float minDist = FLT_MAX;

  while (minDist == FLT_MAX) {
    minNormal = normals[minNormalIdx].n;
    minDist = normals[minNormalIdx].dist;

    glm::vec3 support = findSupportPointMinkowski(a, b, minNormal);
    float dist = glm::dot(minNormal, support);

    if (abs(dist - minDist) > 0.001f) {
      minDist = FLT_MAX;
      std::vector<std::pair<uint32_t, uint32_t>> uniqueEdges;

      for (int i = 0; i < normals.size(); ++i) {
        if (isSameDirection(normals[i].n, support)) {
          uint32_t faceStart = i * 3;

          addUniqueEdge(uniqueEdges, faces, faceStart, faceStart + 1);
          addUniqueEdge(uniqueEdges, faces, faceStart + 1, faceStart + 2);
          addUniqueEdge(uniqueEdges, faces, faceStart + 2, faceStart);

          faces[faceStart + 2] = faces.back(); faces.pop_back();
          faces[faceStart + 1] = faces.back(); faces.pop_back();
          faces[faceStart] = faces.back(); faces.pop_back();

          normals[i] = normals.back(); normals.pop_back();

          i--;
        }
      }

      std::vector<uint32_t> newFaces;
      for (auto [edge0, edge1] : uniqueEdges) {
        newFaces.push_back(edge0);
        newFaces.push_back(edge1);
        newFaces.push_back(polytope.size());
      }

      polytope.push_back(support);

      auto [newNormals, newMinFace, newMinVertexIndices] = getFaceNormals(polytope, newFaces);

      float oldMinDist = FLT_MAX;
      for (uint32_t i = 0; i < normals.size(); ++i) {
        if (normals[i].dist < oldMinDist) {
          oldMinDist = normals[i].dist;
        }
      }

      if (newNormals[newMinFace].dist < oldMinDist) {
        minNormalIdx = newMinFace + normals.size();
        minVertexIndices = newMinVertexIndices + glm::ivec3(faces.size());
      }

      faces.insert(faces.end(), newFaces.begin(), newFaces.end());
      normals.insert(normals.end(), newNormals.begin(), newNormals.end());
    }
  }

  ContactPoint contactPoint = {};
  contactPoint.barycentricCoords = getContactPointBarycentricCoords(
    polytope[faces[minVertexIndices[0]]],
    polytope[faces[minVertexIndices[1]]],
    polytope[faces[minVertexIndices[2]]]);

  for (int i = 0; i < 3; ++i) {
    contactPoint.faceIndicesA[i] = findOriginalVertex(polytope[faces[minVertexIndices[i]]], a);
    contactPoint.faceVerticesA[i] = a.vertices[contactPoint.faceIndicesA[i]];
    contactPoint.faceIndicesB[i] = findOriginalVertex(polytope[faces[minVertexIndices[i]]], b);
    contactPoint.faceVerticesB[i] = b.vertices[contactPoint.faceIndicesB[i]];

    contactPoint.pointA = contactPoint.faceVerticesA[i] * contactPoint.barycentricCoords[i];
    contactPoint.pointB = contactPoint.faceVerticesB[i] * contactPoint.barycentricCoords[i];
  }

  LOG_INFOV("Contact Point A: %s\n", glm::to_string(contactPoint.pointA).c_str());
  LOG_INFOV("Contact Point B: %s\n", glm::to_string(contactPoint.pointB).c_str());

  Collision collision;
  collision.normal = minNormal;
  LOG_INFOV("Contact normal: %s\n", glm::to_string(minNormal).c_str());
  collision.depth = minDist + 0.001f;
  collision.contactPoint = contactPoint;
  collision.bDetectedCollision = true;
  //
  return collision;
}

Collision detectCollision(const CollisionMesh &a, const CollisionMesh &b) {
  GJK gjk = gjkOverlap(a, b);

  if (gjk.bOverlapped) {
    // return epaGetCollision(gjk.simplex, a, b);
    return Collision {
      .bDetectedCollision = true,
    };
  }
  else {
    return {};
  }
}

#if 0
void doSAT(Manifold &manifold, const CollisionMesh &a, const CollisionMesh &b) {
  FaceQuery faceQueryA = queryFaceDirections(a, b);
  if (faceQueryA.separation > 0.0f) {
    // There is a separating axis - no collision
    return;
  }

  FaceQuery faceQueryB = queryFaceDirections(b, a);
  if (faceQueryB.separation > 0.0f) {
    // There is a separating axis - no collision
    return;
  }

  EdgeQuery edgeQuery = queryEdgeDirections(a, b);
  if (edgeQuery.separation > 0.0f) {
    // There is a separating axis - no collision
    return;
  }

  // No separating axis, the meshes overlap
}
#endif

}
