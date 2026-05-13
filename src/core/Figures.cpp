#include "Figures.hpp"
#include "CompositeFigure.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

    void Figure::rotateAroundPoint(sf::Vector2f pivotAbsolute, float deltaRad) {
    sf::Vector2f anchorAbs = getAbsoluteAnchor();
    sf::Vector2f rel = anchorAbs - pivotAbsolute;

    float c = std::cos(deltaRad);
    float s = std::sin(deltaRad);
    sf::Vector2f rotated(rel.x * c - rel.y * s,
                         rel.x * s + rel.y * c);

    sf::Vector2f newAnchorAbs = pivotAbsolute + rotated;

    // anchor хранится в локальных координатах родителя
    if (parentFigure) {
        // редкий случай, пока не нужен
    } else {
        anchor = newAnchorAbs - parentOrigin;
    }

    rotationAngle += deltaRad * math::RAD_TO_DEG;
}
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
    copy->m_focus1Pivot = m_focus1Pivot;
    copy->m_focus2Pivot = m_focus2Pivot;
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
    j["focus1_pivot"] = static_cast<int>(m_focus1Pivot);
    j["focus2_pivot"] = static_cast<int>(m_focus2Pivot);
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
    if (j.contains("focus1_pivot")) {
        m_focus1Pivot = static_cast<FocusPivot>(j["focus1_pivot"].get<int>());
    }
    if (j.contains("focus2_pivot")) {
        m_focus2Pivot = static_cast<FocusPivot>(j["focus2_pivot"].get<int>());
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

void Circle::setFocus1Absolute(sf::Vector2f newAbsF1) {
    if (m_focus1Pivot == FocusPivot::Anchor) {
        sf::Vector2f oldAbsAnchor = getAbsoluteAnchor();
        sf::Vector2f worldDir = newAbsF1 - oldAbsAnchor;
        
        float c = std::sqrt(worldDir.x * worldDir.x + worldDir.y * worldDir.y);
        float requiredAbsRot = std::atan2(-worldDir.y, -worldDir.x) * 180.f / math::PI;
        float parentAbsRot = parentFigure ? parentFigure->getAbsoluteRotation() : 0.f;
        rotationAngle = requiredAbsRot - parentAbsRot;

        m_focusOffset1 = sf::Vector2f(-c, 0.f);
        m_focusOffset2 = sf::Vector2f( c, 0.f);
        
        float a = m_semiMajor;
        if (a < 1.f) a = std::max(m_radiusX, m_radiusY);
        if (c >= a) a = c + 0.1f;
        float b = std::sqrt(a * a - c * c);
        if (b < 1.f) b = 1.f;
        
        m_radiusX = a;
        m_radiusY = b;
        m_semiMajor = a;
        updateVertices();
    } else {
        sf::Vector2f oldAbsF2 = getAbsoluteVertex(m_focusOffset2);
        sf::Vector2f newCenter = (newAbsF1 + oldAbsF2) / 2.f;
        
        if (parentFigure) {
            sf::Vector2f parentAbsAnchor = parentFigure->getAbsoluteAnchor();
            sf::Vector2f delta = newCenter - parentAbsAnchor;
            float parentAbsRot = parentFigure->getAbsoluteRotation();
            sf::Vector2f unrotated = core::math::rotate(delta, -parentAbsRot * core::math::DEG_TO_RAD);
            sf::Vector2f parentAbsScale = parentFigure->getAbsoluteScale();
            anchor = {unrotated.x / parentAbsScale.x, unrotated.y / parentAbsScale.y};
        } else {
            anchor = newCenter - parentOrigin;
        }

        sf::Vector2f dirF2 = oldAbsF2 - newCenter;
        float c = std::sqrt(dirF2.x * dirF2.x + dirF2.y * dirF2.y);
        float requiredAbsRot = std::atan2(dirF2.y, dirF2.x) * 180.f / math::PI;
        float parentAbsRot = parentFigure ? parentFigure->getAbsoluteRotation() : 0.f;
        rotationAngle = requiredAbsRot - parentAbsRot;

        m_focusOffset1 = sf::Vector2f(-c, 0.f);
        m_focusOffset2 = sf::Vector2f( c, 0.f);
        
        float a = m_semiMajor;
        if (a < 1.f) a = std::max(m_radiusX, m_radiusY);
        if (c >= a) a = c + 0.1f;
        float b = std::sqrt(a * a - c * c);
        if (b < 1.f) b = 1.f;
        
        m_radiusX = a;
        m_radiusY = b;
        m_semiMajor = a;
        updateVertices();
    }
}

void Circle::setFocus2Absolute(sf::Vector2f newAbsF2) {
    if (m_focus2Pivot == FocusPivot::Anchor) {
        sf::Vector2f oldAbsAnchor = getAbsoluteAnchor();
        sf::Vector2f worldDir = newAbsF2 - oldAbsAnchor;
        
        float c = std::sqrt(worldDir.x * worldDir.x + worldDir.y * worldDir.y);
        float requiredAbsRot = std::atan2(worldDir.y, worldDir.x) * 180.f / math::PI;
        float parentAbsRot = parentFigure ? parentFigure->getAbsoluteRotation() : 0.f;
        rotationAngle = requiredAbsRot - parentAbsRot;

        m_focusOffset1 = sf::Vector2f(-c, 0.f);
        m_focusOffset2 = sf::Vector2f( c, 0.f);
        
        float a = m_semiMajor;
        if (a < 1.f) a = std::max(m_radiusX, m_radiusY);
        if (c >= a) a = c + 0.1f;
        float b = std::sqrt(a * a - c * c);
        if (b < 1.f) b = 1.f;
        
        m_radiusX = a;
        m_radiusY = b;
        m_semiMajor = a;
        updateVertices();
    } else {
        sf::Vector2f oldAbsF1 = getAbsoluteVertex(m_focusOffset1);
        sf::Vector2f newCenter = (oldAbsF1 + newAbsF2) / 2.f;
        
        if (parentFigure) {
            sf::Vector2f parentAbsAnchor = parentFigure->getAbsoluteAnchor();
            sf::Vector2f delta = newCenter - parentAbsAnchor;
            float parentAbsRot = parentFigure->getAbsoluteRotation();
            sf::Vector2f unrotated = core::math::rotate(delta, -parentAbsRot * core::math::DEG_TO_RAD);
            sf::Vector2f parentAbsScale = parentFigure->getAbsoluteScale();
            anchor = {unrotated.x / parentAbsScale.x, unrotated.y / parentAbsScale.y};
        } else {
            anchor = newCenter - parentOrigin;
        }

        sf::Vector2f dirF2 = newAbsF2 - newCenter;
        float c = std::sqrt(dirF2.x * dirF2.x + dirF2.y * dirF2.y);
        float requiredAbsRot = std::atan2(dirF2.y, dirF2.x) * 180.f / math::PI;
        float parentAbsRot = parentFigure ? parentFigure->getAbsoluteRotation() : 0.f;
        rotationAngle = requiredAbsRot - parentAbsRot;

        m_focusOffset1 = sf::Vector2f(-c, 0.f);
        m_focusOffset2 = sf::Vector2f( c, 0.f);
        
        float a = m_semiMajor;
        if (a < 1.f) a = std::max(m_radiusX, m_radiusY);
        if (c >= a) a = c + 0.1f;
        float b = std::sqrt(a * a - c * c);
        if (b < 1.f) b = 1.f;
        
        m_radiusX = a;
        m_radiusY = b;
        m_semiMajor = a;
        updateVertices();
    }
}

} // namespace core

