#include "Figure.hpp"
#include "MathUtils.hpp"
#include "CompositeFigure.hpp"

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

    std::vector<sf::Vector2f> Figure::getStrokeOuterPoints(bool absolute) const {
        std::vector<sf::Vector2f> pts;
        const auto& verticesRelative = getVertices();
        if (verticesRelative.empty()) return pts;

        size_t n = verticesRelative.size();
        std::vector<sf::Vector2f> V(n);
        for (size_t i = 0; i < n; ++i) {
            V[i] = absolute ? getAbsoluteVertex(verticesRelative[i]) : verticesRelative[i];
            pts.push_back(V[i]);
        }

        if (edges.empty()) return pts;

        float currentScale = 1.0f;
        if (absolute) {
            currentScale = (std::abs(scale.x) + std::abs(scale.y)) / 2.f;
            if (parentFigure) {
                sf::Vector2f absScale = getAbsoluteScale();
                currentScale = (std::abs(absScale.x) + std::abs(absScale.y)) / 2.f;
            }
        }

        std::vector<sf::Vector2f> edgeNormals(n);
        std::vector<sf::Vector2f> edgeDirs(n);
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            sf::Vector2f dir = V[next] - V[i];
            edgeDirs[i] = math::normalize(dir);
            edgeNormals[i] = sf::Vector2f(-edgeDirs[i].y, edgeDirs[i].x);
        }

        float area = 0.f;
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            area += (V[i].x * V[next].y - V[next].x * V[i].y);
        }
        float orientation = (area >= 0.f) ? 1.f : -1.f;
        
        const float MITER_LIMIT = 15.0f;

        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i + n - 1) % n;
            float wPrev = edges[prev < edges.size() ? prev : 0].width * currentScale;
            float wCur  = edges[i   < edges.size() ? i   : 0].width * currentScale;

            if (wPrev <= 0.001f && wCur <= 0.001f) {
                pts.push_back(V[i]);
                continue;
            }

            sf::Vector2f dirPrev = edgeDirs[prev];
            sf::Vector2f dirCur  = edgeDirs[i];
            float crossDirs = dirPrev.x * dirCur.y - dirPrev.y * dirCur.x;
            float turn = orientation * crossDirs;

            sf::Vector2f outerP1 = V[prev] - edgeNormals[prev] * wPrev * orientation;
            sf::Vector2f outerP2 = V[i]    - edgeNormals[i]    * wCur  * orientation;

            sf::Vector2f miterOuter;
            bool hasMiter = core::geometry::lineIntersection(outerP1, dirPrev, outerP2, dirCur, miterOuter);

            if (!hasMiter) {
                pts.push_back(V[i] - edgeNormals[i] * wCur * orientation);
            } else if (turn <= 0.f) {
                // Concave corner
                float miterLen = math::length(miterOuter - V[i]);
                float maxW = std::max(wPrev, wCur);
                const float CONCAVE_LIMIT = 10.0f;
                if (miterLen > CONCAVE_LIMIT * maxW) {
                    sf::Vector2f miterDir = math::normalize(miterOuter - V[i]);
                    pts.push_back(V[i] + miterDir * (CONCAVE_LIMIT * maxW));
                } else {
                    pts.push_back(miterOuter);
                }
            } else {
                // Convex corner: pure miter, no limit — always sharp
                pts.push_back(miterOuter);
            }
        }
        return pts;
    }

    sf::FloatRect Figure::getBoundingBox() const {
        return core::geometry::computeBoundingBox(getStrokeOuterPoints(true));
    }

    sf::FloatRect Figure::getLocalBoundingBox() const {
        return core::geometry::computeBoundingBox(getStrokeOuterPoints(false));
    }

    static float distToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) {
        sf::Vector2f ab = b - a;
        sf::Vector2f ap = p - a;
        float ab_len2 = ab.x * ab.x + ab.y * ab.y;
        if (ab_len2 < 1e-6f) return math::length(ap);
        float t = (ap.x * ab.x + ap.y * ab.y) / ab_len2;
        if (t < 0.f) return math::length(ap);
        if (t > 1.f) return math::length(p - b);
        sf::Vector2f proj = a + ab * t;
        return math::length(p - proj);
    }

    bool Figure::contains(sf::Vector2f point) const {
        std::vector<sf::Vector2f> absVertices;
        const auto& vertices = getVertices();
        absVertices.reserve(vertices.size());
        for (const auto& v : vertices) {
            absVertices.push_back(getAbsoluteVertex(v));
        }
        if (core::geometry::pointInPolygon(point, absVertices)) {
            return true;
        }
        
        float currentScale = (std::abs(scale.x) + std::abs(scale.y)) / 2.f;
        if (parentFigure) {
            sf::Vector2f absScale = getAbsoluteScale();
            currentScale = (std::abs(absScale.x) + std::abs(absScale.y)) / 2.f;
        }
        size_t n = absVertices.size();
        for (size_t i = 0; i < n; ++i) {
            size_t eIdx = i < edges.size() ? i : 0;
            float wCur = edges[eIdx].width * currentScale;
            if (wCur <= 0.001f) continue;
            
            size_t next = (i + 1) % n;
            float dist = distToSegment(point, absVertices[i], absVertices[next]);
            if (dist <= wCur) { // wCur relates to the full outer width since it's fully outside
                return true;
            }
        }
        return false;
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

        // Calculate signed area to handle CCW or CW shapes properly
        float area = 0.f;
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            area += (V[i].x * V[next].y - V[next].x * V[i].y);
        }
        float orientation = (area >= 0.f) ? 1.f : -1.f;
        
        struct Joint {
            sf::Vector2f innerPt;
            sf::Vector2f outerPt;
        };
        std::vector<Joint> joints(n);

        for (size_t i = 0; i < n; ++i) {
            size_t prev = (i + n - 1) % n;

            float wPrev = edges[prev < edges.size() ? prev : 0].width * currentScale;
            float wCur  = edges[i   < edges.size() ? i   : 0].width * currentScale;

            if (wPrev <= 0.001f && wCur <= 0.001f) {
                joints[i] = {V[i], V[i]};
                continue;
            }

            sf::Vector2f dirPrev = edgeDirs[prev];
            sf::Vector2f dirCur  = edgeDirs[i];
            float crossDirs = dirPrev.x * dirCur.y - dirPrev.y * dirCur.x;
            float turn = orientation * crossDirs;

            sf::Vector2f outerP1 = V[prev] - edgeNormals[prev] * wPrev * orientation;
            sf::Vector2f outerP2 = V[i]    - edgeNormals[i]    * wCur  * orientation;

            sf::Vector2f miterOuter;
            bool hasMiter = lineIntersection(outerP1, dirPrev, outerP2, dirCur, miterOuter);

            if (!hasMiter) {
                // Parallel edges — just use normal offset
                joints[i] = {V[i], V[i] - edgeNormals[i] * wCur * orientation};
            } else if (turn <= 0.f) {
                // Concave corner: intersection is behind the vertex, clamp it
                float miterLen = getLength(miterOuter - V[i]);
                float maxW = std::max(wPrev, wCur);
                const float CONCAVE_LIMIT = 10.0f;
                if (miterLen > CONCAVE_LIMIT * maxW) {
                    sf::Vector2f miterDir = normalize(miterOuter - V[i]);
                    miterOuter = V[i] + miterDir * (CONCAVE_LIMIT * maxW);
                }
                joints[i] = {V[i], miterOuter};
            } else {
                // Convex corner: pure miter, no limit, always sharp
                joints[i] = {V[i], miterOuter};
            }
        }

        // Draw edges
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            size_t eIdx = i < edges.size() ? i : 0;

            if (edges[eIdx].width <= 0.001f)
                continue;

            sf::ConvexShape edgeQuad(4);
            edgeQuad.setPoint(0, joints[i].innerPt);
            edgeQuad.setPoint(1, joints[i].outerPt);
            edgeQuad.setPoint(2, joints[next].outerPt);
            edgeQuad.setPoint(3, joints[next].innerPt);
            edgeQuad.setFillColor(edges[eIdx].color);
            target.draw(edgeQuad);
        }
    }

    void Figure::move(sf::Vector2f delta) {
        anchor += delta;
    }

    nlohmann::json Figure::serializeToJson() const {
        nlohmann::json j;
        j["id"] = id;
        j["anchor"] = {anchor.x, anchor.y};
        j["parent_origin"] = {parentOrigin.x, parentOrigin.y};
        j["rotation"] = rotationAngle;
        j["scale"] = {scale.x, scale.y};
        j["fill_color"] = {(int)fillColor.r, (int)fillColor.g, (int)fillColor.b, (int)fillColor.a};

        nlohmann::json edgesArr = nlohmann::json::array();
        for (const auto& e : edges) {
            edgesArr.push_back({
                {"width", e.width},
                {"color", {(int)e.color.r, (int)e.color.g, (int)e.color.b, (int)e.color.a}}
            });
        }
        j["edges"] = edgesArr;

        nlohmann::json lockedSidesArr = nlohmann::json::array();
        for (size_t i = 0; i < lockedSides.size(); ++i) {
            lockedSidesArr.push_back({
                {"locked", lockedSides[i]},
                {"length", lockedLengths[i]}
            });
        }
        j["locked_sides"] = lockedSidesArr;

        nlohmann::json lockedAnglesArr = nlohmann::json::array();
        for (size_t i = 0; i < lockedAngles.size(); ++i) {
            lockedAnglesArr.push_back({
                {"locked", lockedAngles[i]},
                {"value", lockedAngleValues[i]}
            });
        }
        j["locked_angles"] = lockedAnglesArr;

        return j;
    }

    void Figure::deserializeFromJson(const nlohmann::json& j) {
        if (j.contains("id")) id = j["id"].get<std::uint64_t>();
        if (j.contains("anchor")) {
            auto a = j["anchor"];
            anchor = {a[0].get<float>(), a[1].get<float>()};
        }
        if (j.contains("parent_origin")) {
            auto po = j["parent_origin"];
            parentOrigin = {po[0].get<float>(), po[1].get<float>()};
        }
        if (j.contains("rotation")) rotationAngle = j["rotation"].get<float>();
        if (j.contains("scale")) {
            auto s = j["scale"];
            scale = {s[0].get<float>(), s[1].get<float>()};
        }
        if (j.contains("fill_color")) {
            auto fc = j["fill_color"];
            fillColor = sf::Color(fc[0].get<int>(), fc[1].get<int>(), fc[2].get<int>(), fc[3].get<int>());
        }
        if (j.contains("edges")) {
            edges.clear();
            for (const auto& ej : j["edges"]) {
                Edge e;
                e.width = ej["width"].get<float>();
                auto c = ej["color"];
                e.color = sf::Color(c[0].get<int>(), c[1].get<int>(), c[2].get<int>(), c[3].get<int>());
                edges.push_back(e);
            }
        }
        if (j.contains("locked_sides")) {
            lockedSides.clear();
            lockedLengths.clear();
            for (const auto& ls : j["locked_sides"]) {
                lockedSides.push_back(ls["locked"].get<bool>());
                lockedLengths.push_back(ls["length"].get<float>());
            }
        }
        if (j.contains("locked_angles")) {
            lockedAngles.clear();
            lockedAngleValues.clear();
            for (const auto& la : j["locked_angles"]) {
                lockedAngles.push_back(la["locked"].get<bool>());
                lockedAngleValues.push_back(la["value"].get<float>());
            }
        }
    }

} // namespace core