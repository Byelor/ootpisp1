#pragma once

#include <cmath>
#include <SFML/System/Vector2.hpp>

namespace core::math {

    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    inline float length(sf::Vector2f v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    inline sf::Vector2f normalize(sf::Vector2f v, float epsilon = 1e-6f) {
        float len = length(v);
        if (len > epsilon)
            return v / len;
        return sf::Vector2f(0.f, 0.f);
    }

    inline sf::Vector2f perpendicular(sf::Vector2f v) {
        return sf::Vector2f(-v.y, v.x);
    }

    inline float dot(sf::Vector2f a, sf::Vector2f b) {
        return a.x * b.x + a.y * b.y;
    }

    inline float cross(sf::Vector2f a, sf::Vector2f b) {
        return a.x * b.y - a.y * b.x;
    }

    inline sf::Vector2f rotate(sf::Vector2f v, float angle) {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return sf::Vector2f(v.x * c - v.y * s, v.x * s + v.y * c);
    }

    inline sf::Vector2f lerp(sf::Vector2f a, sf::Vector2f b, float t) {
        return a + (b - a) * t;
    }

} // namespace core::math