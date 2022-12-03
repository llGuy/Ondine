#include <map>

#include "Geometry.hpp"
#include "Memory.hpp"
#include "Utils.hpp"
#include "Log.hpp"

namespace Ondine::Game::Physics {

void HalfEdgeMesh::construct(
  FastPolygonList &polygons,
  uint32_t vertexCount, glm::vec3 *vertices) {
  static HalfEdge dummy = {};

  // Allocate all temporary things
  struct Temporary {
    PolygonData *polygons;
    EdgeData *edges;
    HalfEdge *halfEdges;

    // Counters are just for sanity purposes
    uint32_t polygonCount;
    uint32_t edgeCount;
    uint32_t halfEdgeCount;
  };

  Temporary tmp = {};

  // We already know how many polygons there are
  tmp.polygons = lnAllocv<PolygonData>(polygons.polygonCount);
  tmp.halfEdges = lnAllocv<HalfEdge>(polygons.edgeCount);
  // This will be excessive allocation but it's temporary so not big deal
  tmp.edges = lnAllocv<EdgeData>(polygons.edgeCount);


  std::map<std::pair<VertexID, VertexID>, HalfEdgeID> vtxPairToHalfEdge;

  // Proceed with construction
  for (
    uint32_t polygonIndex = 0, *polygon = polygons.begin();
    polygon != polygons.end();
    polygon = polygons.next(polygon), ++polygonIndex) {
    PolygonData *newPolygon = &tmp.polygons[tmp.polygonCount++];

    uint32_t vtxCount = polygons.getPolygonVertexCount(polygon);

    HalfEdge *prev = &dummy;
    uint32_t firstPolygonHalfEdgeIdx = tmp.halfEdgeCount;

    LOG_INFOV("%p vs %p\n", polygon, polygons.end());

    // Create a half edge for each of these
    for (int vIdx = 0; vIdx < vtxCount; ++vIdx) {
      VertexID a = polygon[vIdx];
      VertexID b = polygon[(vIdx + 1) % vtxCount];

      std::pair<VertexID, VertexID> edge = {a, b};

      if (vtxPairToHalfEdge.find(edge) != vtxPairToHalfEdge.end()) {
        // This should never happen - most likely something wrong with polygon construction
        // and the orientation of the faces' vertex indices
        setBreakpoint();
        LOG_ERROR("Error in polygon construction - check orientation of vertex indices\n");
        PANIC_AND_EXIT();
      }
      else {
        // We can allocate a new half edge
        uint32_t hedgeIdx = tmp.halfEdgeCount++;
        HalfEdge *newHalfEdge = &tmp.halfEdges[hedgeIdx];
        newHalfEdge->rootVertex = a;

        // Only set this if the twin was allocated
        std::pair<VertexID, VertexID> twinEdge = {b, a};
        if (auto twin = vtxPairToHalfEdge.find(twinEdge); twin != vtxPairToHalfEdge.end()) {
          // The twin was allocated!
          newHalfEdge->twin = twin->second;
          tmp.halfEdges[twin->second].twin = hedgeIdx;

          // Only call allocate a new "edge" if the twin was already allocated
          EdgeData *newEdge = &tmp.edges[tmp.edgeCount++];
          *newEdge = twin->second;
        }

        prev->next = hedgeIdx;
        prev = newHalfEdge;

        // Insert this half edge into temporary set
        vtxPairToHalfEdge[edge] = hedgeIdx;

        // Just make the polygon point to some random half edge in the polygon
        *newPolygon = hedgeIdx;
      }
    }

    prev->next = firstPolygonHalfEdgeIdx;
  }

  // Copy all these to permanent storage in member pointers
#if 0
  mVertices = flAllocv<glm::vec3>(vertexCount);
  memcpy(mVertices, vertices, sizeof(glm::vec3) * vertexCount);
  mVertexCount = vertexCount;

  mTransformedVertices = flAllocv<glm::vec3>(vertexCount);
#endif

  mPolygons = flAllocv<PolygonData>(tmp.polygonCount);
  memcpy(mPolygons, tmp.polygons, sizeof(PolygonData) * tmp.polygonCount);
  mPolygonCount = tmp.polygonCount;

  mEdges = flAllocv<EdgeData>(tmp.edgeCount);
  memcpy(mEdges, tmp.edges, sizeof(EdgeData) * tmp.edgeCount);
  mEdgeCount = tmp.edgeCount;

  mHalfEdges = flAllocv<HalfEdge>(tmp.halfEdgeCount);
  memcpy(mHalfEdges, tmp.halfEdges, sizeof(HalfEdge) * tmp.halfEdgeCount);
  mHalfEdgeCount = tmp.halfEdgeCount;
}

void HalfEdgeMesh::constructCube() {
  float r = 1.0f;

  glm::vec3 vertices[] = {
    { -r, -r, -r },
    { +r, -r, -r },

    { +r, +r, -r },
    { -r, +r, -r },

    { -r, -r, +r },
    { +r, -r, +r },

    { +r, +r, +r },
    { -r, +r, +r },
  };

  FastPolygonList polygons;
  // 5 per face, 6 faces
  polygons.allocate(5 * 6);

  // anti clockwise
  polygons.addPolygon(4, 0, 1, 2, 3); // -Z
  polygons.addPolygon(4, 7, 6, 5, 4); // +Z
  polygons.addPolygon(4, 3, 2, 6, 7); // +Y

  polygons.addPolygon(4, 4, 5, 1, 0); // -Y
  polygons.addPolygon(4, 5, 6, 2, 1); // +X
  polygons.addPolygon(4, 0, 3, 7, 4); // -X

  construct(polygons, 8, vertices);
}

void HalfEdgeMesh::transform(const glm::mat4 &transform) {
#if 0
  for (int i = 0; i < mVertexCount; ++i) {
    mTransformedVertices[i] = glm::vec3(transform * glm::vec4(mVertices[i], 1.0f));
  }
#endif
}

glm::vec3 HalfEdgeMesh::getFaceNormal(const PolygonData &polygon, glm::vec3 *vertices) const {
  glm::vec3 points[3] = {};

  auto *hEdge = &halfEdge(polygon);
  for (int i = 0; i < 3; ++i) {
    points[i] = vertices[hEdge->rootVertex];
    hEdge = &halfEdge(hEdge->next);
  }

  glm::vec3 a = points[1] - points[0];
  glm::vec3 b = points[2] - points[0];

  return glm::normalize(glm::cross(b, a));
}

glm::vec3 HalfEdgeMesh::getEdgeDirection(const EdgeData &edge, glm::vec3 *vertices) const {
  auto *hEdge = &halfEdge(edge);

  glm::vec3 a = vertices[hEdge->rootVertex];
  glm::vec3 b = vertices[halfEdge(hEdge->next).rootVertex];

  return glm::normalize(b - a);
}

glm::vec3 HalfEdgeMesh::getEdgeOrigin(const EdgeData &edge, glm::vec3 *vertices) const {
  return vertices[halfEdge(edge).rootVertex];
}

Plane HalfEdgeMesh::getPlane(const PolygonData &polygon, glm::vec3 *vertices) const {
  Plane plane = {};
  plane.normal = getFaceNormal(polygon, vertices);
  plane.point = vertices[halfEdge(polygon).rootVertex];
  return plane;
}

uint32_t HalfEdgeMesh::getPolygonCount() const {
  return mPolygonCount;
}

const PolygonData &HalfEdgeMesh::polygon(uint32_t id) const {
  return mPolygons[id];
}

uint32_t HalfEdgeMesh::getEdgeCount() const {
  return mEdgeCount;
}

const EdgeData &HalfEdgeMesh::edge(uint32_t id) const {
  return mEdges[id];
}

// Can pass a polygon data into here - polygon data is just half edge ID
const HalfEdge &HalfEdgeMesh::halfEdge(HalfEdgeID id) const {
  return mHalfEdges[id];
}

#if 0
const glm::vec3 &HalfEdgeMesh::vertex(VertexID id) const {
  return mVertices[id];
}
#endif

}
