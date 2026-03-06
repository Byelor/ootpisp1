#include "GeometryUtils.hpp"
#include "core/MathUtils.hpp"
#include <algorithm>
#include <limits>
#include <stack>

namespace core::geometry {

    bool lineIntersection(sf::Vector2f p1, sf::Vector2f d1,
        sf::Vector2f p2, sf::Vector2f d2,
        sf::Vector2f& intersection) {
        float cross = math::cross(d1, d2);
        if (std::abs(cross) < 1e-6f)
            return false;

        sf::Vector2f diff = p2 - p1;
        float t1 = math::cross(diff, d2) / cross;
        intersection = p1 + d1 * t1;
        return true;
    }

    bool pointInPolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) {
        bool inside = false;
        size_t n = polygon.size();

        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            if (((polygon[i].y > point.y) != (polygon[j].y > point.y)) &&
                (point.x < (polygon[j].x - polygon[i].x) *
                    (point.y - polygon[i].y) /
                    (polygon[j].y - polygon[i].y) + polygon[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }

    void relaxEdges(std::vector<sf::Vector2f>& vertices,
        const std::vector<float>& targetLengths,
        int iterations, float stiffness) {
        size_t n = vertices.size();
        if (n < 3 || targetLengths.size() < n)
            return;

        for (int iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < n; ++i) {
                size_t j = (i + 1) % n;
                sf::Vector2f dir = vertices[j] - vertices[i];
                float dist = math::length(dir);
                if (dist < 1e-6f)
                    continue;

                float err = (dist - targetLengths[i]) / dist;
                sf::Vector2f correction = dir * (stiffness * 0.5f * err);
                vertices[i] += correction;
                vertices[j] -= correction;
            }

            // Центрируем
            sf::Vector2f center(0.f, 0.f);
            for (const auto& v : vertices)
                center += v;
            center /= static_cast<float>(n);
            for (auto& v : vertices)
                v -= center;
        }
    }

    sf::FloatRect computeBoundingBox(const std::vector<sf::Vector2f>& points) {
        if (points.empty())
            return sf::FloatRect(0.f, 0.f, 0.f, 0.f);

        float minX = points[0].x, maxX = points[0].x;
        float minY = points[0].y, maxY = points[0].y;

        for (const auto& p : points) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }

        return sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

} // namespace core::geometry