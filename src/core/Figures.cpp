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
    recalcFociFromRadii();
}

void Circle::updateVertices() {
    const int detail = 64;
    m_vertices.clear();
    for (int i = 0; i < detail; ++i) {
        float angle = 2.f * math::PI * i / static_cast<float>(detail);
        m_vertices.push_back({ m_radiusX * std::cos(angle), m_radiusY * std::sin(angle) });
    }
}

void Circle::recalcFociFromRadii() {
    float a = std::max(m_radiusX, m_radiusY);
    float b = std::min(m_radiusX, m_radiusY);
    float c = (a > b) ? std::sqrt(a * a - b * b) : 0.f;
    if (m_radiusX >= m_radiusY) {
        m_focusOffset1 = sf::Vector2f(-c, 0.f);
        m_focusOffset2 = sf::Vector2f( c, 0.f);
    } else {
        m_focusOffset1 = sf::Vector2f(0.f, -c);
        m_focusOffset2 = sf::Vector2f(0.f,  c);
    }
}

void Circle::recalcRadiiFromFoci() {
    // Determine the foci direction to know which axis is major
    float fx, fy;
    if (m_symmetricFoci) {
        fx = std::abs(m_focusOffset1.x);
        fy = std::abs(m_focusOffset1.y);
    } else {
        fx = std::abs(m_focusOffset2.x - m_focusOffset1.x);
        fy = std::abs(m_focusOffset2.y - m_focusOffset1.y);
    }

    // Calculate focal distance
    float c;
    if (m_symmetricFoci) {
        c = std::sqrt(m_focusOffset1.x * m_focusOffset1.x +
                      m_focusOffset1.y * m_focusOffset1.y);
    } else {
        float dx = m_focusOffset2.x - m_focusOffset1.x;
        float dy = m_focusOffset2.y - m_focusOffset1.y;
        c = std::sqrt(dx * dx + dy * dy) / 2.f;
    }

    float a = std::max(m_radiusX, m_radiusY);

    // Clamp c so it doesn't exceed major radius; also clamp the offsets
    if (c >= a) {
        float s = (a - 0.1f) / c;
        m_focusOffset1.x *= s;
        m_focusOffset1.y *= s;
        m_focusOffset2.x *= s;
        m_focusOffset2.y *= s;
        c = a - 0.1f;
    }

    float b = std::sqrt(a * a - c * c);
    if (b < 1.f) b = 1.f;

    // Determine which axis is major based on foci DIRECTION:
    // Foci more horizontal → X is major (a), Y is minor (b)
    // Foci more vertical   → Y is major (a), X is minor (b)
    // If foci at center (c≈0), keep current orientation
    if (c < 0.001f) {
        // Nearly circular — don't flip axes
        if (m_radiusX >= m_radiusY) {
            m_radiusY = b;
        } else {
            m_radiusX = b;
        }
    } else if (fx >= fy) {
        m_radiusX = a;
        m_radiusY = b;
    } else {
        m_radiusY = a;
        m_radiusX = b;
    }
    updateVertices();
}

void Circle::setRadius(float rx, float ry) {
    m_radiusX = rx;
    m_radiusY = ry;
    updateVertices();
    recalcFociFromRadii();
}

std::unique_ptr<Figure> Circle::clone() const {
    auto copy = std::make_unique<Circle>(m_radiusX, m_radiusY);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->m_focusOffset1 = m_focusOffset1;
    copy->m_focusOffset2 = m_focusOffset2;
    copy->m_symmetricFoci = m_symmetricFoci;
    return copy;
}

nlohmann::json Circle::serializeToJson() const {
    nlohmann::json j = PolylineFigure::serializeToJson();
    j["radius_x"] = m_radiusX;
    j["radius_y"] = m_radiusY;
    j["focus1_x"] = m_focusOffset1.x;
    j["focus1_y"] = m_focusOffset1.y;
    j["focus2_x"] = m_focusOffset2.x;
    j["focus2_y"] = m_focusOffset2.y;
    j["symmetric_foci"] = m_symmetricFoci;
    return j;
}

void Circle::deserializeFromJson(const nlohmann::json& j) {
    PolylineFigure::deserializeFromJson(j);
    float rx = m_radiusX, ry = m_radiusY;
    if (j.contains("radius_x")) rx = j["radius_x"].get<float>();
    if (j.contains("radius_y")) ry = j["radius_y"].get<float>();
    setRadius(rx, ry);

    // Load stored foci if present, otherwise keep computed defaults
    if (j.contains("focus1_x") && j.contains("focus1_y")) {
        m_focusOffset1.x = j["focus1_x"].get<float>();
        m_focusOffset1.y = j["focus1_y"].get<float>();
    }
    if (j.contains("focus2_x") && j.contains("focus2_y")) {
        m_focusOffset2.x = j["focus2_x"].get<float>();
        m_focusOffset2.y = j["focus2_y"].get<float>();
    }
    if (j.contains("symmetric_foci")) {
        m_symmetricFoci = j["symmetric_foci"].get<bool>();
    }
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
    return m_focusOffset1;
}

sf::Vector2f Circle::getFocus2() const {
    return m_focusOffset2;
}

void Circle::setFocus1(sf::Vector2f offset) {
    m_focusOffset1 = offset;
    if (m_symmetricFoci) {
        m_focusOffset2 = sf::Vector2f(-offset.x, -offset.y);
    }
    recalcRadiiFromFoci();
}

void Circle::setFocus2(sf::Vector2f offset) {
    m_focusOffset2 = offset;
    if (m_symmetricFoci) {
        m_focusOffset1 = sf::Vector2f(-offset.x, -offset.y);
    }
    recalcRadiiFromFoci();
}

void Circle::setSymmetricFoci(bool sym) {
    m_symmetricFoci = sym;
    if (sym) {
        // When enabling symmetry, mirror Focus2 from Focus1
        m_focusOffset2 = sf::Vector2f(-m_focusOffset1.x, -m_focusOffset1.y);
        recalcRadiiFromFoci();
    }
}

} // namespace core

