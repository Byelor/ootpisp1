#pragma once

#include "Figure.hpp"

namespace core {

class Circle : public Figure {
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
    void setAnchorKeepAbsolute(sf::Vector2f newAnchor) override;

    enum class FocusPivot { Anchor, OtherFocus };

    float getFocalDistance() const;
    void setFocalDistance(float c);
    sf::Vector2f getFocus1() const;
    sf::Vector2f getFocus2() const;

    FocusPivot getFocus1Pivot() const { return m_focus1Pivot; }
    void setFocus1Pivot(FocusPivot p) { m_focus1Pivot = p; }

    FocusPivot getFocus2Pivot() const { return m_focus2Pivot; }
    void setFocus2Pivot(FocusPivot p) { m_focus2Pivot = p; }

    void setFocus1Absolute(sf::Vector2f absolutePos);
    void setFocus2Absolute(sf::Vector2f absolutePos);

private:
    void updateVertices();
    void recalcFociFromRadii();

    float m_radiusX;
    float m_radiusY;
    sf::Vector2f m_focusOffset1{0.f, 0.f};
    sf::Vector2f m_focusOffset2{0.f, 0.f};
    FocusPivot m_focus1Pivot = FocusPivot::Anchor;
    FocusPivot m_focus2Pivot = FocusPivot::Anchor;
    float m_semiMajor = 0.f;
};

} // namespace core