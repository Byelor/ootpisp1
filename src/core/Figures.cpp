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

void Circle::serialize(std::ostream& out, int indent) const {
    PolylineFigure::serialize(out, indent);
    std::string pad(indent, ' ');
    out << pad << "radius_x " << m_radiusX << "\n";
    out << pad << "radius_y " << m_radiusY << "\n";
}

bool Circle::deserialize(const std::string& prop, std::istream& in) {
    if (prop == "radius_x") {
        float rx; in >> rx;
        setRadius(rx, m_radiusY);
        return true;
    } else if (prop == "radius_y") {
        float ry; in >> ry;
        setRadius(m_radiusX, ry);
        return true;
    }
    return PolylineFigure::deserialize(prop, in);
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

} // namespace core
