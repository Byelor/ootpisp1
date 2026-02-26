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
Hexagon::Hexagon(float radius) : m_radius(radius) {
  edges.resize(6);
  m_vertices.resize(6);
  for (int i = 0; i < 6; ++i) {
    float angle = i * 60.f * 3.14159f / 180.f;
    m_vertices[i] = {m_radius * std::cos(angle), m_radius * std::sin(angle)};
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

} // namespace core
