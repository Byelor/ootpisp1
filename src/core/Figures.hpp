#pragma once

#include "PolylineFigure.hpp"

namespace core {

class Rectangle : public PolylineFigure {
public:
    Rectangle(float width, float height);
    std::string typeName() const override { return "rectangle"; }
    const char* getSideName(int idx) const override;
    void setSideLengths(const std::vector<float>& lengths) override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_width, m_height;
};

class Triangle : public PolylineFigure {
public:
    Triangle(float base, float height);
    std::string typeName() const override { return "triangle"; }
    const char* getSideName(int idx) const override;
    void setSideLengths(const std::vector<float>& lengths) override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_base, m_height;
};

class Hexagon : public PolylineFigure {
public:
    Hexagon(float width, float height);
    std::string typeName() const override { return "hexagon"; }
    const char* getSideName(int idx) const override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_width, m_height;
};

class Rhombus : public PolylineFigure {
public:
    Rhombus(float width, float height);
    std::string typeName() const override { return "rhombus"; }
    const char* getSideName(int idx) const override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_width, m_height;
};

class Trapezoid : public PolylineFigure {
public:
    Trapezoid(float topWidth, float bottomWidth, float height);
    std::string typeName() const override { return "trapezoid"; }
    const char* getSideName(int idx) const override;
    void setSideLengths(const std::vector<float>& lengths) override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_topWidth, m_bottomWidth, m_height;
};

class Circle : public PolylineFigure {
public:
    Circle(float radiusX, float radiusY);
    std::string typeName() const override { return "circle"; }
    std::unique_ptr<Figure> clone() const override;
    bool hasSideLengths() const override { return false; }
    bool hasUniformEdge() const override { return true; }
    void draw(sf::RenderTarget& target) const override;
    
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
