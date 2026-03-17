#pragma once

#include "Figure.hpp"
#include <string>
#include <vector>
#include <memory>

namespace core {

class CompositeFigure : public Figure {
public:
    enum class Preset {
        None,
        Rectangle,
        Triangle,
        Hexagon,
        Rhombus,
        Trapezoid,
        Circle
    };

    std::string figureName;
    Preset preset = Preset::None;

    struct Child {
        std::unique_ptr<Figure> figure;
        sf::Vector2f localOffset;
        float localRotation = 0.f;
    };

    std::vector<Child> children;

    CompositeFigure() = default;
    explicit CompositeFigure(std::vector<sf::Vector2f> vertices, std::string name = "Custom");

    // Factory methods for base shapes
    static std::unique_ptr<CompositeFigure> createRectangle(float width, float height);
    static std::unique_ptr<CompositeFigure> createTriangle(float base, float height);
    static std::unique_ptr<CompositeFigure> createHexagon(float width, float height);
    static std::unique_ptr<CompositeFigure> createRhombus(float width, float height);
    static std::unique_ptr<CompositeFigure> createTrapezoid(float topWidth, float bottomWidth, float height);
    static std::unique_ptr<CompositeFigure> createCircle(float radiusX, float radiusY);

    // Overrides
    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;
    
    // Bounding box override to account for children
    const std::vector<sf::Vector2f>& getVertices() const override;

    bool hasSideLengths() const override;
    void setSideLengths(const std::vector<float>& lengths) override;
    const char* getSideName(int idx) const override;
    
    std::string typeName() const override;
    std::unique_ptr<Figure> clone() const override;
    void applyScale() override;
    bool hasUniformEdge() const override;

    // Angle modification for polylines
    void setEdgeAngle(int edgeIdx, float angleDeg);
    float getEdgeAngle(int edgeIdx) const;

private:
    float m_cachedWidth = 0.f;
    float m_cachedHeight = 0.f;
    float m_cachedTopWidth = 0.f;
    float m_cachedBottomWidth = 0.f;

    mutable std::vector<sf::Vector2f> m_cachedVerts;
    
    void updateCachedDimensions();
};

} // namespace core
