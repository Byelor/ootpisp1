#pragma once

#include "Figure.hpp"

namespace core {

class Rectangle : public Figure {
public:
  Rectangle(float width, float height);

  bool hasSideLengths() const override { return true; }
  void setSideLengths(const std::vector<float> &lengths) override;
  const char *getSideName(int idx) const override {
    static const char *n[] = {"Top", "Right", "Bottom", "Left"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
  }

private:
  float m_width;
  float m_height;
};

class Triangle : public Figure {
public:
  Triangle(float base, float height);

  bool hasSideLengths() const override { return true; }
  void setSideLengths(const std::vector<float> &lengths) override;
  const char *getSideName(int idx) const override {
    static const char *n[] = {"Bottom", "Right side", "Left side"};
    return (idx >= 0 && idx < 3) ? n[idx] : "Side";
  }

private:
  float m_base;
  float m_height;
};

class Hexagon : public Figure {
public:
  Hexagon(float width, float height);

  bool hasSideLengths() const override { return true; }
  void setSideLengths(const std::vector<float> &lengths) override;
  const char *getSideName(int idx) const override {
    static const char *n[] = {"Side 1", "Side 2", "Side 3",
                              "Side 4", "Side 5", "Side 6"};
    return (idx >= 0 && idx < 6) ? n[idx] : "Side";
  }

private:
  float m_width;
  float m_height;
};

class Rhombus : public Figure {
public:
  Rhombus(float width, float height);

  bool hasSideLengths() const override { return true; }
  void setSideLengths(const std::vector<float> &lengths) override;
  const char *getSideName(int idx) const override {
    static const char *n[] = {"Top-R", "Bottom-R", "Bottom-L", "Top-L"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
  }

private:
  float m_width;
  float m_height;
};

class Trapezoid : public Figure {
public:
  Trapezoid(float topWidth, float bottomWidth, float height);

  bool hasSideLengths() const override { return true; }
  void setSideLengths(const std::vector<float> &lengths) override;
  const char *getSideName(int idx) const override {
    static const char *n[] = {"Top", "Right leg", "Bottom", "Left leg"};
    return (idx >= 0 && idx < 4) ? n[idx] : "Side";
  }

private:
  float m_topWidth;
  float m_bottomWidth;
  float m_height;
};

class Circle : public Figure {
public:
  Circle(float radiusX, float radiusY);
  bool hasUniformEdge() const override { return true; }

private:
  float m_radiusX;
  float m_radiusY;
};

} // namespace core
