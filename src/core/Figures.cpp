#include "Figures.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

// ─── Circle 
Circle::Circle(float radiusX, float radiusY) : m_radiusX(radiusX), m_radiusY(radiusY) {
    figureName = "Circle";
    m_semiMajor = std::max(radiusX, radiusY);
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
    // Foci always along X axis; visual orientation is via rotationAngle
    m_focusOffset1 = sf::Vector2f(-c, 0.f);
    m_focusOffset2 = sf::Vector2f( c, 0.f);
}

void Circle::recalcRadiiFromFoci() {
    // Determine focus direction in world-like coordinates
    // (m_focusOffset holds the raw direction before normalization)
    float fx, fy;
    if (m_symmetricFoci) {
        fx = m_focusOffset1.x;
        fy = m_focusOffset1.y;
    } else {
        fx = (m_focusOffset2.x - m_focusOffset1.x) / 2.f;
        fy = (m_focusOffset2.y - m_focusOffset1.y) / 2.f;
    }

    // 1. Compute rotation angle from focus direction
    // Focus1 is stored as (-c, 0), so rotating (-c, 0) by θ must give (fx, fy):
    //   -c·cos(θ) = fx,  -c·sin(θ) = fy  →  θ = atan2(-fy, -fx)
    float focusAngle = std::atan2(-fy, -fx); // radians
    rotationAngle = focusAngle * 180.f / math::PI;

    // 2. Compute focal distance
    float c = std::sqrt(fx * fx + fy * fy);

    // 3. Use stored semi-major axis to prevent drift
    float a = m_semiMajor;
    if (a < 1.f) a = std::max(m_radiusX, m_radiusY); // fallback

    // 4. Clamp c so it doesn't exceed major radius
    if (c >= a) {
        c = a - 0.1f;
    }

    // 5. Compute minor semi-axis
    float b = std::sqrt(a * a - c * c);
    if (b < 1.f) b = 1.f;

    // 6. Set radii: major axis always along X
    m_radiusX = a;
    m_radiusY = b;

    // 7. Normalize foci offsets to lie along X axis
    m_focusOffset1 = sf::Vector2f(-c, 0.f);
    m_focusOffset2 = sf::Vector2f( c, 0.f);

    updateVertices();
}

void Circle::setRadius(float rx, float ry) {
    m_radiusX = rx;
    m_radiusY = ry;
    m_semiMajor = std::max(rx, ry);
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
    copy->m_semiMajor = m_semiMajor;
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
    j["semi_major"] = m_semiMajor;
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
    if (j.contains("semi_major")) {
        m_semiMajor = j["semi_major"].get<float>();
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
    float a = m_semiMajor;
    if (a < 1.f) a = std::max(m_radiusX, m_radiusY);
    // Clamp c so it doesn't exceed the major axis
    if (c >= a) c = a - 0.1f;
    float b = std::sqrt(a * a - c * c);
    if (b < 1.f) b = 1.f;
    // Major axis always along X; rotation handles orientation
    setRadius(a, b);
}

sf::Vector2f Circle::getFocus1() const {
    return m_focusOffset1;
}

sf::Vector2f Circle::getFocus2() const {
    return m_focusOffset2;
}

void Circle::setFocus1(sf::Vector2f worldDir) {
    // worldDir is the world-space direction from center to focus.
    // Callers must pass (absPos - absoluteAnchor) directly.
    m_focusOffset1 = worldDir;
    if (m_symmetricFoci) {
        m_focusOffset2 = sf::Vector2f(-worldDir.x, -worldDir.y);
    }
    recalcRadiiFromFoci();
}

void Circle::setFocus2(sf::Vector2f worldDir) {
    // worldDir is the world-space direction from center to focus.
    m_focusOffset2 = worldDir;
    if (m_symmetricFoci) {
        m_focusOffset1 = sf::Vector2f(-worldDir.x, -worldDir.y);
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

