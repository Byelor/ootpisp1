#pragma once

#include "core/Figure.hpp"
#include "core/Scene.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

namespace ui {

class CreateFigureModal {
public:
  void render(core::Scene &scene);

  bool isOpen() const { return m_open; }
  void open(sf::Vector2f pos);
  void close() { m_open = false; }

private:
  void resetDefaults();
  void reinitEdges();
  std::unique_ptr<core::Figure> createConfiguredFigure();

  bool m_open = false;
  sf::Vector2f m_createPos;
  int m_figureType = 0;

  // For circle only
  float m_radiusX = 50.f;
  float m_radiusY = 50.f;

  float m_fillColor[4] = {0.6f, 0.6f, 0.6f, 1.0f};

  struct TempEdge {
    float length = 100.f; // side length (for polygons)
    float width = 2.0f;   // stroke thickness
    float color[4] = {0.f, 0.f, 0.f, 1.f};
  };
  std::vector<TempEdge> m_edges;
};

} // namespace ui
