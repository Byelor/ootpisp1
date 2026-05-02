#pragma once
#include "Figure.hpp"
#include <SFML/Graphics.hpp>
#include <string>

namespace core {

/// Фигура, задаваемая произвольным набором вершин (ломаная, автоматически замыкается).
/// Базовые фигуры (Rectangle, Triangle, …) станут подклассами PolylineFigure.
class PolylineFigure : public Figure {
public:
    std::string figureName;

    explicit PolylineFigure(std::vector<sf::Vector2f> vertices, std::string name = "Custom");

    bool hasSideLengths()  const override { return true; }
    void setSideLengths(const std::vector<float>& lengths) override;
    const char* getSideName(int idx) const override;

    void setVertices(const std::vector<sf::Vector2f>& verts) { m_vertices = verts; }

    std::string typeName() const override { return "polyline"; }
    std::unique_ptr<Figure> clone() const override;

    nlohmann::json serializeToJson() const override;
    void deserializeFromJson(const nlohmann::json& j) override;

    /// Get the interior angle at vertex i (degrees, 0..360)
    float getVertexAngle(int vertIdx) const;
    /// Set the interior angle at vertex i by rotating the outgoing edge
    void setVertexAngle(int vertIdx, float angleDeg);

public:
    PolylineFigure() = default; // для подклассов (базовых фигур)
};

} // namespace core
