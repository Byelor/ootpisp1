#include "Figures.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

// ─── Rectangle 
Rectangle::Rectangle(float width, float height) : m_width(width), m_height(height) {
    figureName = "Rectangle";
    m_vertices = {
        {-width / 2.f, -height / 2.f},
        {width / 2.f, -height / 2.f},
        {width / 2.f, height / 2.f},
        {-width / 2.f, height / 2.f}
    };
    edges.resize(4);
}

const char* Rectangle::getSideName(int idx) const {
    static const char* n[] = {"Top", "Right", "Bottom", "Left"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
}

void Rectangle::setSideLengths(const std::vector<float>& lengths) {
    if (lengths.size() < 4) return;
    float top = std::max(1.f, lengths[0]);
    float left = std::max(1.f, lengths[3]);
    m_width = top;
    m_height = left;
    m_vertices = {
        {-m_width / 2.f, -m_height / 2.f},
        {m_width / 2.f, -m_height / 2.f},
        {m_width / 2.f, m_height / 2.f},
        {-m_width / 2.f, m_height / 2.f}
    };
}

std::unique_ptr<Figure> Rectangle::clone() const {
    auto copy = std::make_unique<Rectangle>(m_width, m_height);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    return copy;
}

// ─── Triangle 
Triangle::Triangle(float base, float height) : m_base(base), m_height(height) {
    figureName = "Triangle";
    m_vertices = {
        {0.f, -height / 2.f},
        {base / 2.f, height / 2.f},
        {-base / 2.f, height / 2.f}
    };
    edges.resize(3);
}

const char* Triangle::getSideName(int idx) const {
    static const char* n[] = {"Right side", "Bottom", "Left side"};
    return (idx >= 0 && idx < 3) ? n[idx] : "Side";
}

void Triangle::setSideLengths(const std::vector<float>& lengths) {
    if (lengths.size() < 3) return;
    float b = std::max(1.f, lengths[0]); 
    float a = std::max(1.f, lengths[1]); 
    float c = std::max(1.f, lengths[2]); 
    float sumBC = b + c, sumAC = a + c, sumAB = a + b;
    if (a >= sumBC) a = sumBC - 1.f;
    if (b >= sumAC) b = sumAC - 1.f;
    if (c >= sumAB) c = sumAB - 1.f;
    float x2 = (c * c + a * a - b * b) / (2.f * a);
    float y2sq = c * c - x2 * x2;
    float y2 = -std::sqrt(std::max(0.f, y2sq)); 
    float cx = (0.f + a + x2) / 3.f;
    float cy = (0.f + 0.f + y2) / 3.f;
    m_vertices = {
        {x2 - cx, y2 - cy},  
        {a - cx, 0.f - cy},  
        {0.f - cx, 0.f - cy} 
    };
    m_base = a;
    m_height = std::abs(y2);
}

std::unique_ptr<Figure> Triangle::clone() const {
    auto copy = std::make_unique<Triangle>(m_base, m_height);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    return copy;
}

// ─── Hexagon 
Hexagon::Hexagon(float width, float height) : m_width(width), m_height(height) {
    figureName = "Hexagon";
    edges.resize(6);
    m_vertices.resize(6);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 60.f * math::PI / 180.f;
        m_vertices[i] = { (width / 2.f) * std::cos(angle), (height / 2.f) * std::sin(angle) };
    }
}

const char* Hexagon::getSideName(int idx) const {
    return "Side";
}

std::unique_ptr<Figure> Hexagon::clone() const {
    auto copy = std::make_unique<Hexagon>(m_width, m_height);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    return copy;
}

// ─── Rhombus 
Rhombus::Rhombus(float width, float height) : m_width(width), m_height(height) {
    figureName = "Rhombus";
    edges.resize(4);
    m_vertices = {
        {0.f, -height / 2.f},
        {width / 2.f, 0.f},
        {0.f, height / 2.f},
        {-width / 2.f, 0.f}
    };
}

const char* Rhombus::getSideName(int idx) const {
    static const char* n[] = {"Top-R", "Bottom-R", "Bottom-L", "Top-L"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
}

std::unique_ptr<Figure> Rhombus::clone() const {
    auto copy = std::make_unique<Rhombus>(m_width, m_height);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    return copy;
}

// ─── Trapezoid 
Trapezoid::Trapezoid(float topWidth, float bottomWidth, float height)
    : m_topWidth(topWidth), m_bottomWidth(bottomWidth), m_height(height) {
    figureName = "Trapezoid";
    edges.resize(4);
    m_vertices = {
        {-topWidth / 2.f, -height / 2.f},
        {topWidth / 2.f, -height / 2.f},
        {bottomWidth / 2.f, height / 2.f},
        {-bottomWidth / 2.f, height / 2.f}
    };
}

const char* Trapezoid::getSideName(int idx) const {
    static const char* n[] = {"Top", "Right leg", "Bottom", "Left leg"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
}

void Trapezoid::setSideLengths(const std::vector<float>& lengths) {
    if (lengths.size() < 4) return;
    float top = std::max(1.f, lengths[0]);
    float right = std::max(1.f, lengths[1]);
    float bottom = std::max(1.f, lengths[2]);
    float left = std::max(1.f, lengths[3]);
    float D = top - bottom;
    float dx_left, dx_right;
    if (std::abs(D) < 1e-4f) {
        dx_left = 0.f; dx_right = 0.f;
    } else {
        dx_left = (right * right - D * D - left * left) / (2.f * D);
        dx_right = dx_left + D;
    }
    float h = std::sqrt(std::max(1.f, left * left - dx_left * dx_left));
    float BLx = 0.f, BLy = 0.f;
    float BRx = bottom, BRy = 0.f;
    float TRx = BRx + dx_right, TRy = -h;
    float TLx = BLx + dx_left, TLy = -h;
    float cx = (BLx + BRx + TRx + TLx) / 4.f;
    float cy = (BLy + BRy + TRy + TLy) / 4.f;
    m_vertices = {
        {TLx - cx, TLy - cy},
        {TRx - cx, TRy - cy},
        {BRx - cx, BRy - cy},
        {BLx - cx, BLy - cy}
    };
    m_topWidth = top;
    m_bottomWidth = bottom;
    m_height = h;
}

std::unique_ptr<Figure> Trapezoid::clone() const {
    auto copy = std::make_unique<Trapezoid>(m_topWidth, m_bottomWidth, m_height);
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    return copy;
}

// ─── Circle 
Circle::Circle(float radiusX, float radiusY) : m_radiusX(radiusX), m_radiusY(radiusY) {
    figureName = "Circle";
    // Circle uses one virtual edge entry for outline style.
    edges.resize(1);
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

sf::FloatRect Circle::getLocalBoundingBox() const {
    float stroke = edges.empty() ? 0.f : edges[0].width;
    float rx = m_radiusX + stroke;
    float ry = m_radiusY + stroke;
    return sf::FloatRect(-rx, -ry, rx * 2.f, ry * 2.f);
}

sf::FloatRect Circle::getBoundingBox() const {
    sf::Vector2f absAnchor = getAbsoluteAnchor();
    sf::Vector2f absScale = getAbsoluteScale();
    float rotRad = getAbsoluteRotation() * math::DEG_TO_RAD;
    
    float stroke = edges.empty() ? 0.f : edges[0].width;
    
    
    float a = m_radiusX * std::abs(absScale.x) + stroke;
    float b = m_radiusY * std::abs(absScale.y) + stroke;
    float cosPhi = std::cos(rotRad);
    float sinPhi = std::sin(rotRad);
    
    float halfWidth = std::sqrt(a*a*cosPhi*cosPhi + b*b*sinPhi*sinPhi);
    float halfHeight = std::sqrt(a*a*sinPhi*sinPhi + b*b*cosPhi*cosPhi);
    
    return sf::FloatRect(absAnchor.x - halfWidth, absAnchor.y - halfHeight, halfWidth * 2.f, halfHeight * 2.f);
}

bool Circle::contains(sf::Vector2f point) const {
    sf::Vector2f absAnchor = getAbsoluteAnchor();
    sf::Vector2f absScale = getAbsoluteScale();
    float rotRad = getAbsoluteRotation() * math::DEG_TO_RAD;
    
    sf::Vector2f relPoint = point - absAnchor;
    
    sf::Vector2f localPoint;
    localPoint.x = relPoint.x * std::cos(-rotRad) - relPoint.y * std::sin(-rotRad);
    localPoint.y = relPoint.x * std::sin(-rotRad) + relPoint.y * std::cos(-rotRad);
    
    // Scale the radii
    float stroke = edges.empty() ? 0.f : edges[0].width;
    float a = m_radiusX * std::abs(absScale.x) + stroke / 2.f;
    float b = m_radiusY * std::abs(absScale.y) + stroke / 2.f;
    
    if (a < 1e-6f || b < 1e-6f) return false;
    
    float normX = localPoint.x / a;
    float normY = localPoint.y / b;
    return (normX * normX + normY * normY) <= 1.0f;
}


void Circle::draw(sf::RenderTarget& target) const {
    sf::CircleShape shape(1.f);
    shape.setPointCount(100); 
    
    sf::Vector2f absAnchor = getAbsoluteAnchor();
    shape.setPosition(absAnchor);
    shape.setOrigin({1.f, 1.f});
    shape.setRotation(getAbsoluteRotation());
    
    sf::Vector2f absScale = getAbsoluteScale();
    shape.setScale({absScale.x * m_radiusX, absScale.y * m_radiusY});
    
    shape.setFillColor(fillColor);
    
    if (!edges.empty() && edges[0].width > 0.001f) {
        shape.setOutlineColor(edges[0].color);
        // Inverse scale the outline thickness so it remains uniform despite the shape's scale
        float avgScale = (std::abs(absScale.x * m_radiusX) + std::abs(absScale.y * m_radiusY)) / 2.f;
        shape.setOutlineThickness(edges[0].width / avgScale);
    } else {
        shape.setOutlineThickness(0.f);
    }
    
    target.draw(shape);
    
}


} // namespace core
