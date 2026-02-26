#include "Figures.hpp"
#include <cmath>

namespace core {

// Helper to check if point is in polygon
static bool isPointInPolygon(sf::Vector2f point,
                             const std::vector<sf::Vector2f> &vertices,
                             sf::Vector2f anchor) {
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
static float dot(sf::Vector2f a, sf::Vector2f b) {
  return a.x * b.x + a.y * b.y;
}
static sf::Vector2f getNormal(sf::Vector2f v) {
  // Left-hand normal (outward if vertices are clockwise)
  return sf::Vector2f(-v.y, v.x);
}

// Function to find intersection of two lines: p1 + t1*d1 and p2 + t2*d2
// Returns true if intersection found, false if parallel
static bool lineIntersection(sf::Vector2f p1, sf::Vector2f d1, sf::Vector2f p2,
                             sf::Vector2f d2, sf::Vector2f &intersection) {
  float cross = d1.x * d2.y - d1.y * d2.x;
  if (std::abs(cross) < 1e-6f)
    return false; // Parallell

  sf::Vector2f diff = p2 - p1;
  float t1 = (diff.x * d2.y - diff.y * d2.x) / cross;
  intersection = p1 + d1 * t1;
  return true;
}

// Draw shape with properly joined thick edges using sf::ConvexShape (or
// VertexArray if needed, but ConvexShape is easier per edge quad)
static void basicDraw(const Figure &fig, sf::RenderTarget &target) {
  auto verticesRelative = fig.getVertices();
  if (verticesRelative.empty())
    return;
  int n = verticesRelative.size();

  // 1. Draw Fill
  sf::ConvexShape fillShape(n);
  for (int i = 0; i < n; ++i) {
    fillShape.setPoint(i, verticesRelative[i]);
  }
  fillShape.setPosition(fig.anchor);
  fillShape.setFillColor(fig.fillColor);
  target.draw(fillShape);

  // 2. Draw Edges matching "edge" styles
  if (fig.edges.empty())
    return;

  // Convert to absolute vertices
  std::vector<sf::Vector2f> V(n);
  for (int i = 0; i < n; ++i) {
    V[i] = fig.anchor + verticesRelative[i];
  }

  // Prepare arrays for outer polygon (to use sf::VertexArray for drawing all
  // edges as a single TriangleStrip/Quads) Actually, miter limits can get
  // tricky, so we'll draw each edge as a separate polygon. For each vertex
  // V[i], it connects edge {i-1} and edge {i}.
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
    sf::Vector2f outerPt2; // same as outerPt1 for miter, different for bevel
    bool isBevel;
  };
  std::vector<Joint> joints(n);

  const float MITER_LIMIT = 4.0f; // Limit relative to edge width

  for (int i = 0; i < n; ++i) {
    int prev = (i - 1 + n) % n;

    float wPrev = fig.edges[prev < fig.edges.size() ? prev : 0].width;
    float wCur = fig.edges[i < fig.edges.size() ? i : 0].width;

    if (wPrev == 0.f && wCur == 0.f) {
      joints[i] = {V[i], V[i], V[i], false};
      continue;
    }

    // Half widths
    float hwPrev = wPrev / 2.f;
    float hwCur = wCur / 2.f;

    // Lines for outer edges
    sf::Vector2f outerLineP1 = V[prev] + edgeNormals[prev] * hwPrev;
    sf::Vector2f outerLineD1 = edgeDirs[prev];

    sf::Vector2f outerLineP2 = V[i] + edgeNormals[i] * hwCur;
    sf::Vector2f outerLineD2 = edgeDirs[i];

    // Lines for inner edges
    sf::Vector2f innerLineP1 = V[prev] - edgeNormals[prev] * hwPrev;
    sf::Vector2f innerLineP2 = V[i] - edgeNormals[i] * hwCur;

    sf::Vector2f miterOuter;
    sf::Vector2f miterInner;

    bool hasMiterOuter = lineIntersection(outerLineP1, outerLineD1, outerLineP2,
                                          outerLineD2, miterOuter);
    bool hasMiterInner = lineIntersection(innerLineP1, outerLineD1, innerLineP2,
                                          outerLineD2, miterInner);

    if (!hasMiterOuter || !hasMiterInner) {
      // Parallel or straight line
      miterOuter = V[i] + edgeNormals[i] * hwCur;
      miterInner = V[i] - edgeNormals[i] * hwCur;
      joints[i] = {miterInner, miterOuter, miterOuter, false};
    } else {
      // Check miter limit
      float miterLen = getLength(miterOuter - V[i]);
      float maxW = std::max(wPrev, wCur);
      if (miterLen > maxW * MITER_LIMIT) {
        // Bevel join
        sf::Vector2f bevelOuter1 =
            outerLineP1 +
            outerLineD1 *
                getLength(V[i] - V[prev]); // Approx perpendicular pt at V[i]
        sf::Vector2f bevelOuter2 =
            outerLineP2; // Already starts at correct perpendicular offset

        // Better bevel calculation:
        // find where the perpendicular from V[i] hits outer lines
        bevelOuter1 = V[i] + edgeNormals[prev] * hwPrev;
        bevelOuter2 = V[i] + edgeNormals[i] * hwCur;

        joints[i] = {miterInner, bevelOuter1, bevelOuter2, true};
      } else {
        joints[i] = {miterInner, miterOuter, miterOuter, false};
      }
    }
  }

  // Now draw each edge
  for (int i = 0; i < n; ++i) {
    int next = (i + 1) % n;
    int eIdx = i < fig.edges.size() ? i : 0;

    if (fig.edges[eIdx].width <= 0.001f)
      continue;

    sf::ConvexShape edgeQuad;

    if (joints[i].isBevel) {
      // For bevel, we might have a gap. It's usually easier to just draw the
      // quads and an extra triangle for the bevel Draw the edge quad
      edgeQuad.setPointCount(4);
      // Our quad is defined by joint[i] and joint[next]
      edgeQuad.setPoint(0, joints[i].innerPt);
      edgeQuad.setPoint(1, joints[i].outerPt2);
      edgeQuad.setPoint(2, joints[next].outerPt1);
      edgeQuad.setPoint(3, joints[next].innerPt);
    } else {
      edgeQuad.setPointCount(4);
      edgeQuad.setPoint(0, joints[i].innerPt);
      edgeQuad.setPoint(1, joints[i].outerPt1); // == outerPt2
      edgeQuad.setPoint(2, joints[next].outerPt1);
      edgeQuad.setPoint(3, joints[next].innerPt);
    }

    edgeQuad.setFillColor(fig.edges[eIdx].color);
    target.draw(edgeQuad);

    // If the START joint was a bevel, we need to fill the gap
    if (joints[i].isBevel) {
      sf::ConvexShape bevelTri(3);
      bevelTri.setPoint(0, joints[i].innerPt);
      bevelTri.setPoint(1, joints[i].outerPt1);
      bevelTri.setPoint(2, joints[i].outerPt2);
      bevelTri.setFillColor(
          fig.edges[eIdx]
              .color); // Color of the current edge arbitrarily (plan says
                       // largest edge width, but this is simpler)
      target.draw(bevelTri);
    }
  }
}

// Rectangle
Rectangle::Rectangle(float width, float height)
    : m_width(width), m_height(height) {
  edges.resize(4);
}
std::vector<sf::Vector2f> Rectangle::getVertices() const {
  return {{-m_width / 2.f, -m_height / 2.f},
          {m_width / 2.f, -m_height / 2.f},
          {m_width / 2.f, m_height / 2.f},
          {-m_width / 2.f, m_height / 2.f}};
}
void Rectangle::draw(sf::RenderTarget &target) const {
  basicDraw(*this, target);
}
bool Rectangle::contains(sf::Vector2f point) const {
  return isPointInPolygon(point, getVertices(), anchor);
}

// Triangle
Triangle::Triangle(float base, float height) : m_base(base), m_height(height) {
  edges.resize(3);
}
std::vector<sf::Vector2f> Triangle::getVertices() const {
  return {{0.f, -m_height / 2.f},
          {m_base / 2.f, m_height / 2.f},
          {-m_base / 2.f, m_height / 2.f}};
}
void Triangle::draw(sf::RenderTarget &target) const {
  basicDraw(*this, target);
}
bool Triangle::contains(sf::Vector2f point) const {
  return isPointInPolygon(point, getVertices(), anchor);
}

// Hexagon
Hexagon::Hexagon(float radius) : m_radius(radius) { edges.resize(6); }
std::vector<sf::Vector2f> Hexagon::getVertices() const {
  std::vector<sf::Vector2f> pts(6);
  for (int i = 0; i < 6; ++i) {
    float angle = i * 60.f * 3.14159f / 180.f;
    pts[i] = {m_radius * std::cos(angle), m_radius * std::sin(angle)};
  }
  return pts;
}
void Hexagon::draw(sf::RenderTarget &target) const { basicDraw(*this, target); }
bool Hexagon::contains(sf::Vector2f point) const {
  return isPointInPolygon(point, getVertices(), anchor);
}

// Rhombus
Rhombus::Rhombus(float width, float height) : m_width(width), m_height(height) {
  edges.resize(4);
}
std::vector<sf::Vector2f> Rhombus::getVertices() const {
  return {{0.f, -m_height / 2.f},
          {m_width / 2.f, 0.f},
          {0.f, m_height / 2.f},
          {-m_width / 2.f, 0.f}};
}
void Rhombus::draw(sf::RenderTarget &target) const { basicDraw(*this, target); }
bool Rhombus::contains(sf::Vector2f point) const {
  return isPointInPolygon(point, getVertices(), anchor);
}

// Trapezoid
Trapezoid::Trapezoid(float topWidth, float bottomWidth, float height)
    : m_topWidth(topWidth), m_bottomWidth(bottomWidth), m_height(height) {
  edges.resize(4);
}
std::vector<sf::Vector2f> Trapezoid::getVertices() const {
  return {{-m_topWidth / 2.f, -m_height / 2.f},
          {m_topWidth / 2.f, -m_height / 2.f},
          {m_bottomWidth / 2.f, m_height / 2.f},
          {-m_bottomWidth / 2.f, m_height / 2.f}};
}
void Trapezoid::draw(sf::RenderTarget &target) const {
  basicDraw(*this, target);
}
bool Trapezoid::contains(sf::Vector2f point) const {
  return isPointInPolygon(point, getVertices(), anchor);
}

} // namespace core
