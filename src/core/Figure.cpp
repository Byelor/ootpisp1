#include "Figure.hpp"
#include "MathUtils.hpp"
#include "CompositeFigure.hpp"
#include "GeometryUtils.hpp"
#include "../utils/GeometryUtils.hpp"
#include <cmath>

namespace core {

    sf::Vector2f Figure::getAbsoluteAnchor() const {
        if (parentFigure) {
            sf::Vector2f pScale = parentFigure->getAbsoluteScale();
            sf::Vector2f scaledAnchor(anchor.x * pScale.x, anchor.y * pScale.y);
            sf::Vector2f rotatedAnchor = math::rotate(scaledAnchor, parentFigure->getAbsoluteRotation() * math::DEG_TO_RAD);
            return parentFigure->getAbsoluteAnchor() + rotatedAnchor;
        }
        return parentOrigin + anchor;
    }

    float Figure::getAbsoluteRotation() const {
        if (parentFigure) {
            return parentFigure->getAbsoluteRotation() + rotationAngle;
        }
        return rotationAngle;
    }

    sf::Vector2f Figure::getAbsoluteScale() const {
        if (parentFigure) {
            sf::Vector2f pScale = parentFigure->getAbsoluteScale();
            return {scale.x * pScale.x, scale.y * pScale.y};
        }
        return scale;
    }

    sf::Vector2f Figure::getAbsoluteVertex(sf::Vector2f relative) const {
        sf::Vector2f absScale = getAbsoluteScale();
        sf::Vector2f scaled(relative.x * absScale.x, relative.y * absScale.y);
        sf::Vector2f rotated = math::rotate(scaled, getAbsoluteRotation() * math::DEG_TO_RAD);
        return getAbsoluteAnchor() + rotated;
    }

    sf::FloatRect Figure::getBoundingBox() const {
        const auto& vertices = getVertices();
        if (vertices.empty()) {
            return sf::FloatRect(anchor.x, anchor.y, 0.f, 0.f);
        }

        std::vector<sf::Vector2f> absVertices;
        absVertices.reserve(vertices.size());
        for (const auto& v : vertices) {
            absVertices.push_back(getAbsoluteVertex(v));
        }

        return core::geometry::computeBoundingBox(absVertices);
    }

    sf::FloatRect Figure::getLocalBoundingBox() const {
        return core::geometry::computeBoundingBox(getVertices());
    }

    bool Figure::contains(sf::Vector2f point) const {
        std::vector<sf::Vector2f> absVertices;
        const auto& vertices = getVertices();
        absVertices.reserve(vertices.size());
        for (const auto& v : vertices) {
            absVertices.push_back(getAbsoluteVertex(v));
        }
        return core::geometry::pointInPolygon(point, absVertices);
    }

    void Figure::resetAnchor() {
        sf::FloatRect bounds = getLocalBoundingBox();
        sf::Vector2f localCenter(bounds.left + bounds.width / 2.0f,
            bounds.top + bounds.height / 2.0f);

        if (std::abs(localCenter.x) < 0.001f && std::abs(localCenter.y) < 0.001f) {
            return;
        }

        sf::Vector2f rotated = math::rotate(
            sf::Vector2f(localCenter.x * scale.x, localCenter.y * scale.y),
            rotationAngle * math::DEG_TO_RAD);

        anchor += rotated;

        for (auto& v : m_vertices) {
            v -= localCenter;
        }
    }

    void Figure::setAnchorKeepAbsolute(sf::Vector2f newAnchor) {
        if (math::length(newAnchor - anchor) < 0.001f) {
            return;
        }

        sf::Vector2f deltaA = newAnchor - anchor;
        sf::Vector2f rotated = math::rotate(deltaA, -rotationAngle * math::DEG_TO_RAD);

        float vx = (scale.x != 0.f) ? (rotated.x / scale.x) : 0.f;
        float vy = (scale.y != 0.f) ? (rotated.y / scale.y) : 0.f;

        anchor = newAnchor;
        for (auto& v : m_vertices) {
            v.x -= vx;
            v.y -= vy;
        }
    }

    void Figure::applyScale() {
        for (auto& v : m_vertices) {
            v.x *= scale.x;
            v.y *= scale.y;
        }
        scale = sf::Vector2f(1.f, 1.f);
    }

    std::vector<float> Figure::getSideLengths() const {
        std::vector<float> lengths;
        const auto& verts = getVertices();
        size_t n = verts.size();
        if (n < 2) return lengths;

        for (size_t i = 0; i < n; ++i) {
            size_t j = (i + 1) % n;
            sf::Vector2f a(verts[i].x * scale.x, verts[i].y * scale.y);
            sf::Vector2f b(verts[j].x * scale.x, verts[j].y * scale.y);
            lengths.push_back(math::length(b - a));
        }
        return lengths;
    }

    void Figure::applyGenericSideLengths(const std::vector<float>& lengths) {
        core::geometry::relaxEdges(m_vertices, lengths, 1000, 0.5f);
    }

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

    void Figure::draw(sf::RenderTarget& target) const {
        const auto& verticesRelative = getVertices();
        if (verticesRelative.empty()) return;

        size_t n = verticesRelative.size();

        // Fill
        sf::ConvexShape fillShape(static_cast<std::size_t>(n));
        for (size_t i = 0; i < n; ++i) {
            fillShape.setPoint(i, verticesRelative[i]);
        }
        fillShape.setPosition(getAbsoluteAnchor());
        fillShape.setRotation(getAbsoluteRotation());
        fillShape.setScale(getAbsoluteScale());
        fillShape.setFillColor(fillColor);
        target.draw(fillShape);

        if (edges.empty()) return;

        // Scale thickness correctly
        float currentScale = (std::abs(scale.x) + std::abs(scale.y)) / 2.f;
        if (parentFigure) {
            sf::Vector2f absScale = getAbsoluteScale();
            currentScale = (std::abs(absScale.x) + std::abs(absScale.y)) / 2.f;
        }

        std::vector<sf::Vector2f> V(n);
        for (size_t i = 0; i < n; ++i) {
            V[i] = getAbsoluteVertex(verticesRelative[i]);
        }

        std::vector<sf::Vector2f> edgeNormals(n);
        std::vector<sf::Vector2f> edgeDirs(n);
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
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

        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i + n - 1) % n;

            float wPrev = edges[prev < edges.size() ? prev : 0].width * currentScale;
            float wCur = edges[i < edges.size() ? i : 0].width * currentScale;

            if (wPrev <= 0.001f && wCur <= 0.001f) {
                joints[i] = {V[i], V[i], V[i], false};
                continue;
            }

            // To draw the stroke completely OUTSIDE the shape:
            // edgeNormals point INSIDE the shape.
            // Inner contour is exactly the vertices (V).
            // Outer contour is shifted by -normal * width.
            
            sf::Vector2f innerLineP1 = V[prev];
            sf::Vector2f innerLineP2 = V[i];
            
            sf::Vector2f outerLineP1 = V[prev] - edgeNormals[prev] * wPrev;
            sf::Vector2f outerLineD1 = edgeDirs[prev];

            sf::Vector2f outerLineP2 = V[i] - edgeNormals[i] * wCur;
            sf::Vector2f outerLineD2 = edgeDirs[i];

            sf::Vector2f miterOuter;
            sf::Vector2f miterInner;

            // hasMiterOuter = intersection of the OUTSIDE shifted lines (the exterior spike)
            bool hasMiterOuter = lineIntersection(outerLineP1, outerLineD1, outerLineP2,
                                                  outerLineD2, miterOuter);
            // hasMiterInner = intersection of the exact inside polygon lines (exactly V[i])
            bool hasMiterInner = lineIntersection(innerLineP1, outerLineD1, innerLineP2,
                                                  outerLineD2, miterInner);

            if (!hasMiterOuter || !hasMiterInner) {
                miterOuter = V[i] - edgeNormals[i] * wCur;
                miterInner = V[i];
                joints[i] = {miterInner, miterOuter, miterOuter, false};
            } else {
                float miterLen = getLength(miterOuter - V[i]);
                float maxW = std::max(wPrev, wCur);
                if (miterLen > maxW * MITER_LIMIT) {
                    // Bevel the exterior spike by capping it
                    sf::Vector2f bevelOuter1 = V[i] - edgeNormals[prev] * wPrev;
                    sf::Vector2f bevelOuter2 = V[i] - edgeNormals[i] * wCur;
                    joints[i] = {miterInner, bevelOuter1, bevelOuter2, true};
                } else {
                    joints[i] = {miterInner, miterOuter, miterOuter, false};
                }
            }
        }

        // Draw edges
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            size_t eIdx = i < edges.size() ? i : 0;

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

    void Figure::move(sf::Vector2f delta) {
        anchor += delta;
    }

} // namespace core