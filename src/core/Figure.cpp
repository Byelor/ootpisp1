#include "Figure.hpp"
#include <cmath>

namespace core {

// Math helpers
static float getLength(sf::Vector2f v) {
  return std::sqrt(v.x * v.x + v.y * v.y);
}
static sf::Vector2f normalize(sf::Vector2f v) {
  float len = getLength(v);
  if (len > 0.0001f)
    return v / len;
  return sf::Vector2f(0.f, 0.f);
}
static sf::Vector2f getNormal(sf::Vector2f v) {
  return sf::Vector2f(-v.y, v.x);
}
static bool lineIntersection(sf::Vector2f p1, sf::Vector2f d1, sf::Vector2f p2,
                             sf::Vector2f d2, sf::Vector2f &intersection) {
  float cross = d1.x * d2.y - d1.y * d2.x;
  if (std::abs(cross) < 1e-6f)
    return false; // Parallel

  sf::Vector2f diff = p2 - p1;
  float t1 = (diff.x * d2.y - diff.y * d2.x) / cross;
  intersection = p1 + d1 * t1;
  return true;
}

sf::FloatRect Figure::getBoundingBox() const {
  auto vertices = getVertices();
  if (vertices.empty()) {
    return sf::FloatRect(anchor.x, anchor.y, 0.f, 0.f);
  }

  float minX = vertices[0].x;
  float maxX = vertices[0].x;
  float minY = vertices[0].y;
  float maxY = vertices[0].y;

  for (const auto &v : vertices) {
    if (v.x < minX)
      minX = v.x;
    if (v.x > maxX)
      maxX = v.x;
    if (v.y < minY)
      minY = v.y;
    if (v.y > maxY)
      maxY = v.y;
  }

  // Convert relative to absolute
  return sf::FloatRect(anchor.x + minX, anchor.y + minY, maxX - minX,
                       maxY - minY);
}

void Figure::move(sf::Vector2f delta) { anchor += delta; }

bool Figure::contains(sf::Vector2f point) const {
  const auto &vertices = getVertices();
  bool c = false;
  int nvert = vertices.size();
  for (int i = 0, j = nvert - 1; i < nvert; j = i++) {
    sf::Vector2f vi = anchor + vertices[i];
    sf::Vector2f vj = anchor + vertices[j];
    if (((vi.y > point.y) != (vj.y > point.y)) &&
        (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x))
      c = !c;
  }
  return c;
}

void Figure::draw(sf::RenderTarget &target) const {
  auto verticesRelative = getVertices();
  if (verticesRelative.empty())
    return;
  int n = verticesRelative.size();

  // 1. Draw Fill
  sf::ConvexShape fillShape(n);
  for (int i = 0; i < n; ++i) {
    fillShape.setPoint(i, verticesRelative[i]);
  }
  fillShape.setPosition(anchor);
  fillShape.setFillColor(fillColor);
  target.draw(fillShape);

  // 2. Draw Edges matching "edge" styles
  if (edges.empty())
    return;

  // Convert to absolute vertices
  std::vector<sf::Vector2f> V(n);
  for (int i = 0; i < n; ++i) {
    V[i] = anchor + verticesRelative[i];
  }

  std::vector<sf::Vector2f> edgeNormals(n);
  std::vector<sf::Vector2f> edgeDirs(n);
  for (int i = 0; i < n; ++i) {
    int next = (i + 1) % n;
    sf::Vector2f dir = V[next] - V[i];
    edgeDirs[i] = normalize(dir);
    edgeNormals[i] = getNormal(edgeDirs[i]);
  }

  struct Joint {
    sf::Vector2f innerPt;
    sf::Vector2f outerPt1;
    sf::Vector2f outerPt2;
    bool isBevel;
  };
  std::vector<Joint> joints(n);

  const float MITER_LIMIT = 4.0f;

  for (int i = 0; i < n; ++i) {
    int prev = (i - 1 + n) % n;

    float wPrev = edges[prev < edges.size() ? prev : 0].width;
    float wCur = edges[i < edges.size() ? i : 0].width;

    if (wPrev == 0.f && wCur == 0.f) {
      joints[i] = {V[i], V[i], V[i], false};
      continue;
    }

    float hwPrev = wPrev / 2.f;
    float hwCur = wCur / 2.f;

    sf::Vector2f outerLineP1 = V[prev] + edgeNormals[prev] * hwPrev;
    sf::Vector2f outerLineD1 = edgeDirs[prev];

    sf::Vector2f outerLineP2 = V[i] + edgeNormals[i] * hwCur;
    sf::Vector2f outerLineD2 = edgeDirs[i];

    sf::Vector2f innerLineP1 = V[prev] - edgeNormals[prev] * hwPrev;
    sf::Vector2f innerLineP2 = V[i] - edgeNormals[i] * hwCur;

    sf::Vector2f miterOuter;
    sf::Vector2f miterInner;

    bool hasMiterOuter = lineIntersection(outerLineP1, outerLineD1, outerLineP2,
                                          outerLineD2, miterOuter);
    bool hasMiterInner = lineIntersection(innerLineP1, outerLineD1, innerLineP2,
                                          outerLineD2, miterInner);

    if (!hasMiterOuter || !hasMiterInner) {
      miterOuter = V[i] + edgeNormals[i] * hwCur;
      miterInner = V[i] - edgeNormals[i] * hwCur;
      joints[i] = {miterInner, miterOuter, miterOuter, false};
    } else {
      float miterLen = getLength(miterOuter - V[i]);
      float maxW = std::max(wPrev, wCur);
      if (miterLen > maxW * MITER_LIMIT) {
        sf::Vector2f bevelOuter1 = V[i] + edgeNormals[prev] * hwPrev;
        sf::Vector2f bevelOuter2 = V[i] + edgeNormals[i] * hwCur;
        joints[i] = {miterInner, bevelOuter1, bevelOuter2, true};
      } else {
        joints[i] = {miterInner, miterOuter, miterOuter, false};
      }
    }
  }

  // Draw edges
  for (int i = 0; i < n; ++i) {
    int next = (i + 1) % n;
    int eIdx = i < edges.size() ? i : 0;

    if (edges[eIdx].width <= 0.001f)
      continue;

    sf::ConvexShape edgeQuad;

    if (joints[i].isBevel) {
      edgeQuad.setPointCount(4);
      edgeQuad.setPoint(0, joints[i].innerPt);
      edgeQuad.setPoint(1, joints[i].outerPt2);
      edgeQuad.setPoint(2, joints[next].outerPt1);
      edgeQuad.setPoint(3, joints[next].innerPt);
    } else {
      edgeQuad.setPointCount(4);
      edgeQuad.setPoint(0, joints[i].innerPt);
      edgeQuad.setPoint(1, joints[i].outerPt1);
      edgeQuad.setPoint(2, joints[next].outerPt1);
      edgeQuad.setPoint(3, joints[next].innerPt);
    }

    edgeQuad.setFillColor(edges[eIdx].color);
    target.draw(edgeQuad);

    if (joints[i].isBevel) {
      sf::ConvexShape bevelTri(3);
      bevelTri.setPoint(0, joints[i].innerPt);
      bevelTri.setPoint(1, joints[i].outerPt1);
      bevelTri.setPoint(2, joints[i].outerPt2);
      bevelTri.setFillColor(edges[eIdx].color);
      target.draw(bevelTri);
    }
  }
}

} // namespace core
