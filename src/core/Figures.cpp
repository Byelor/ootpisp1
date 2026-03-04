#include "Figures.hpp"
#include <cmath>

namespace core {

// Rectangle
Rectangle::Rectangle(float width, float height)
    : m_width(width), m_height(height) {
  edges.resize(4);
  m_vertices = {{-m_width / 2.f, -m_height / 2.f},
                {m_width / 2.f, -m_height / 2.f},
                {m_width / 2.f, m_height / 2.f},
                {-m_width / 2.f, m_height / 2.f}};
}

// Triangle
Triangle::Triangle(float base, float height) : m_base(base), m_height(height) {
  edges.resize(3);
  m_vertices = {{0.f, -m_height / 2.f},
                {m_base / 2.f, m_height / 2.f},
                {-m_base / 2.f, m_height / 2.f}};
}

// Hexagon
Hexagon::Hexagon(float width, float height) : m_width(width), m_height(height) {
  edges.resize(6);
  m_vertices.resize(6);
  for (int i = 0; i < 6; ++i) {
    float angle = i * 60.f * 3.14159f / 180.f;
    m_vertices[i] = {(m_width / 2.f) * std::cos(angle),
                     (m_height / 2.f) * std::sin(angle)};
  }
}

// Rhombus
Rhombus::Rhombus(float width, float height) : m_width(width), m_height(height) {
  edges.resize(4);
  m_vertices = {{0.f, -m_height / 2.f},
                {m_width / 2.f, 0.f},
                {0.f, m_height / 2.f},
                {-m_width / 2.f, 0.f}};
}

// Trapezoid
Trapezoid::Trapezoid(float topWidth, float bottomWidth, float height)
    : m_topWidth(topWidth), m_bottomWidth(bottomWidth), m_height(height) {
  edges.resize(4);
  m_vertices = {{-m_topWidth / 2.f, -m_height / 2.f},
                {m_topWidth / 2.f, -m_height / 2.f},
                {m_bottomWidth / 2.f, m_height / 2.f},
                {-m_bottomWidth / 2.f, m_height / 2.f}};
}

// Circle (Ellipse)
Circle::Circle(float radiusX, float radiusY)
    : m_radiusX(radiusX), m_radiusY(radiusY) {
  int segments = 60;
  edges.resize(1);
  m_vertices.resize(segments);
  for (int i = 0; i < segments; ++i) {
    float angle = i * 2.f * 3.14159265f / segments;
    m_vertices[i] = {m_radiusX * std::cos(angle), m_radiusY * std::sin(angle)};
  }
}

} // namespace core
