#include "Figure.hpp"
#include "MathUtils.hpp"
#include "CompositeFigure.hpp"
#include "utils/GeometryUtils.hpp"
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

        return geometry::computeBoundingBox(absVertices);
    }

    sf::FloatRect Figure::getLocalBoundingBox() const {
        return geometry::computeBoundingBox(getVertices());
    }

    bool Figure::contains(sf::Vector2f point) const {
        std::vector<sf::Vector2f> absVertices;
        const auto& vertices = getVertices();
        absVertices.reserve(vertices.size());
        for (const auto& v : vertices) {
            absVertices.push_back(getAbsoluteVertex(v));
        }
        return geometry::pointInPolygon(point, absVertices);
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
        geometry::relaxEdges(m_vertices, lengths, 1000, 0.5f);
    }

    void Figure::draw(sf::RenderTarget& target) const {
        const auto& verticesRelative = getVertices();
        if (verticesRelative.empty()) return;

        size_t n = verticesRelative.size();

        //  
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

        //  
        std::vector<sf::Vector2f> V(n);
        for (size_t i = 0; i < n; ++i) {
            V[i] = getAbsoluteVertex(verticesRelative[i]);
        }

        //      
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            size_t eIdx = i < edges.size() ? i : 0;

            if (edges[eIdx].width <= 0.001f) continue;

            sf::Vector2f dir = math::normalize(V[next] - V[i]);
            sf::Vector2f normal = math::perpendicular(dir);
            float halfWidth = edges[eIdx].width / 2.f;

            sf::ConvexShape edgeQuad(4);
            edgeQuad.setPoint(0, V[i] - normal * halfWidth);
            edgeQuad.setPoint(1, V[i] + normal * halfWidth);
            edgeQuad.setPoint(2, V[next] + normal * halfWidth);
            edgeQuad.setPoint(3, V[next] - normal * halfWidth);
            edgeQuad.setFillColor(edges[eIdx].color);

            target.draw(edgeQuad);
        }
    }

    void Figure::move(sf::Vector2f delta) {
        anchor += delta;
    }

} // namespace core