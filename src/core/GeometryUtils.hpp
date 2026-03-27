#pragma once

#include "MathUtils.hpp"
#include <SFML/Graphics.hpp>
#include <vector>

namespace core::geometry {

    // Generates a VertexArray for a polyline with specified thickness and miter joins.
    // If closed is true, it connects the last vertex back to the first.
    inline sf::VertexArray generateThickPolyline(const std::vector<sf::Vector2f>& points, 
                                                 const std::vector<sf::Color>& colors,
                                                 const std::vector<float>& thicknesses,
                                                 bool closed) 
    {
        if (points.size() < 2) return sf::VertexArray(sf::TriangleStrip, 0);

        size_t n = points.size();
        size_t numSegments = closed ? n : (n - 1);
        sf::VertexArray varray(sf::TriangleStrip, 0);

        for (size_t i = 0; i < numSegments; ++i) {
            sf::Vector2f p0, p1, p2, p3;
            if (closed) {
                p0 = points[(i + n - 1) % n];
                p1 = points[i];
                p2 = points[(i + 1) % n];
                p3 = points[(i + 2) % n];
            } else {
                p0 = (i == 0) ? points[0] - (points[1] - points[0]) : points[i - 1];
                p1 = points[i];
                p2 = points[i + 1];
                p3 = (i == numSegments - 1) ? points[i + 1] + (points[i + 1] - points[i]) : points[i + 2];
            }

            sf::Vector2f dir1 = math::normalize(p1 - p0);
            sf::Vector2f dir2 = math::normalize(p2 - p1);
            sf::Vector2f dir3 = math::normalize(p3 - p2);

            sf::Vector2f n1 = sf::Vector2f(-dir1.y, dir1.x);
            sf::Vector2f n2 = sf::Vector2f(-dir2.y, dir2.x);
            sf::Vector2f n3 = sf::Vector2f(-dir3.y, dir3.x);

            sf::Vector2f miter1 = math::normalize(n1 + n2);
            float length1 = 1.0f / math::dot(miter1, n2);
            miter1 *= length1;

            sf::Vector2f miter2 = math::normalize(n2 + n3);
            float length2 = 1.0f / math::dot(miter2, n2);
            miter2 *= length2;

            float thickness = thicknesses[i % thicknesses.size()];
            float halfThick = thickness / 2.0f;

            // Cap miter length to prevent extreme spikes on sharp angles
            const float miterLimit = 3.f * halfThick;
            if (math::length(miter1 * halfThick) > miterLimit) {
                miter1 = n2;
            }
            if (math::length(miter2 * halfThick) > miterLimit) {
                miter2 = n2;
            }

            sf::Vector2f v1 = p1 + miter1 * halfThick;
            sf::Vector2f v2 = p1 - miter1 * halfThick;
            sf::Vector2f v3 = p2 + miter2 * halfThick;
            sf::Vector2f v4 = p2 - miter2 * halfThick;

            sf::Color col = colors[i % colors.size()];

            if (i > 0) {
                // Degenerate triangle to stitch segments
                varray.append(sf::Vertex(varray[varray.getVertexCount() - 1].position, col));
                varray.append(sf::Vertex(v1, col));
            }

            varray.append(sf::Vertex(v1, col));
            varray.append(sf::Vertex(v2, col));
            varray.append(sf::Vertex(v3, col));
            varray.append(sf::Vertex(v4, col));
        }

        return varray;
    }

} // namespace core::geometry
