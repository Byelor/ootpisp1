#pragma once

#include "Figure.hpp"

namespace core {

class Rectangle : public Figure {
public:
  Rectangle(float width, float height);

private:
  float m_width;
  float m_height;
};

class Triangle : public Figure {
public:
  Triangle(float base, float height);

private:
  float m_base;
  float m_height;
};

class Hexagon : public Figure {
public:
  Hexagon(float radius);

private:
  float m_radius;
};

class Rhombus : public Figure {
public:
  Rhombus(float width, float height);

private:
  float m_width;
  float m_height;
};

class Trapezoid : public Figure {
public:
  Trapezoid(float topWidth, float bottomWidth, float height);

private:
  float m_topWidth;
  float m_bottomWidth;
  float m_height;
};

} // namespace core
