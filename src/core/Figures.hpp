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
    void setFocus1(sf::Vector2f offset);
    void setFocus2(sf::Vector2f offset);

    bool isSymmetricFoci() const { return m_symmetricFoci; }
    void setSymmetricFoci(bool sym);
    
private:
    void updateVertices();
    void recalcFociFromRadii();
    void recalcRadiiFromFoci();
    float m_radiusX, m_radiusY;
    sf::Vector2f m_focusOffset1{0.f, 0.f};
    sf::Vector2f m_focusOffset2{0.f, 0.f};
    bool m_symmetricFoci = true;
};

} // namespace core

