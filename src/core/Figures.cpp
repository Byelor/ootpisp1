#include "Figures.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

// ─── Circle 
Circle::Circle(float radiusX, float radiusY) : m_radiusX(radiusX), m_radiusY(radiusY) {
    figureName = "Circle";
    // Circle uses one virtual edge entry for outline style.
    edges.resize(1);
    updateVertices();
}

void Circle::updateVertices() {
    const int detail = 64;
    m_vertices.clear();
    for (int i = 0; i < detail; ++i) {
        float angle = 2.f * math::PI * i / static_cast<float>(detail);
        m_vertices.push_back({ m_radiusX * std::cos(angle), m_radiusY * std::sin(angle) });
    }
}

void Circle::setRadius(float rx, float ry) {
    m_radiusX = rx;
    m_radiusY = ry;
    updateVertices();
}

std::unique_ptr<Figure> Circle::clone() const {
    auto copy = std::make_unique<Circle>(m_radiusX, m_radiusY);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    return copy;
}

nlohmann::json Circle::serializeToJson() const {
    nlohmann::json j = PolylineFigure::serializeToJson();
    j["radius_x"] = m_radiusX;
    j["radius_y"] = m_radiusY;
    return j;
}

void Circle::deserializeFromJson(const nlohmann::json& j) {
    PolylineFigure::deserializeFromJson(j);
    float rx = m_radiusX, ry = m_radiusY;
    if (j.contains("radius_x")) rx = j["radius_x"].get<float>();
    if (j.contains("radius_y")) ry = j["radius_y"].get<float>();
    setRadius(rx, ry);
}

void Circle::draw(sf::RenderTarget& target) const {
    PolylineFigure::draw(target);
}

sf::FloatRect Circle::getLocalBoundingBox() const {
    return Figure::getLocalBoundingBox();
}

sf::FloatRect Circle::getBoundingBox() const {
    return Figure::getBoundingBox();
}

// ─── Foci methods ─────────────────────────────────────────────────────────────

float Circle::getFocalDistance() const {
    float a = std::max(m_radiusX, m_radiusY);
    float b = std::min(m_radiusX, m_radiusY);
    if (a <= b) return 0.f;
    return std::sqrt(a * a - b * b);
}

void Circle::setFocalDistance(float c) {
    if (c < 0.f) c = 0.f;
    float a = std::max(m_radiusX, m_radiusY);
    // Clamp c so it doesn't exceed the major axis
    if (c >= a) c = a - 0.1f;
    float b = std::sqrt(a * a - c * c);
    if (b < 1.f) b = 1.f;
    // Recalculate the minor axis
    if (m_radiusX >= m_radiusY) {
        setRadius(m_radiusX, b);
    } else {
        setRadius(b, m_radiusY);
    }
}

sf::Vector2f Circle::getFocus1() const {
    float c = getFocalDistance();
    if (m_radiusX >= m_radiusY) {
        return sf::Vector2f(-c, 0.f);
    } else {
        return sf::Vector2f(0.f, -c);
    }
}

sf::Vector2f Circle::getFocus2() const {
    float c = getFocalDistance();
    if (m_radiusX >= m_radiusY) {
        return sf::Vector2f(c, 0.f);
    } else {
        return sf::Vector2f(0.f, c);
    }
}

} // namespace core
