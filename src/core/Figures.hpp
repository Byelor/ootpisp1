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
    nlohmann::json serializeToJson() const override;
    void deserializeFromJson(const nlohmann::json& j) override;
    
    sf::FloatRect getBoundingBox() const override;
    sf::FloatRect getLocalBoundingBox() const override;

    float getRadiusX() const { return m_radiusX; }
    float getRadiusY() const { return m_radiusY; }
    void setRadius(float rx, float ry);

    // Foci methods
    float getFocalDistance() const;
    void setFocalDistance(float c);
    sf::Vector2f getFocus1() const;
    sf::Vector2f getFocus2() const;
    
private:
    void updateVertices();
    float m_radiusX, m_radiusY;
};

} // namespace core

