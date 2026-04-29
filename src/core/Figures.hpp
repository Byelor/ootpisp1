#pragma once

#include "PolylineFigure.hpp"

namespace core {

class Circle : public PolylineFigure {
public:
    Circle(float radiusX, float radiusY);
    std::string typeName() const override { return "circle"; }
    std::unique_ptr<Figure> clone() const override;
    bool hasSideLengths() const override { return false; }
    bool hasUniformEdge() const override { return true; }
    void draw(sf::RenderTarget& target) const override;
    void serialize(std::ostream& out, int indent) const override;
    bool deserialize(const std::string& prop, std::istream& in) override;
    
    sf::FloatRect getBoundingBox() const override;
    sf::FloatRect getLocalBoundingBox() const override;

    float getRadiusX() const { return m_radiusX; }
    float getRadiusY() const { return m_radiusY; }
    void setRadius(float rx, float ry);
    
private:
    void updateVertices();
    float m_radiusX, m_radiusY;
};

} // namespace core
