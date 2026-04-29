#pragma once

#include "core/Figure.hpp"
#include "core/Scene.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

#include "Toolbar.hpp"

namespace ui {

class CreateFigureModal {
public:
  // render() now needs mutable refs so it can add templates to the registry
  void render(core::Scene& scene,
              std::vector<Toolbar::CustomTool>& customTools,
              std::vector<std::unique_ptr<core::Figure>>& userRegistry);

  bool isOpen() const { return m_open; }
  void open(sf::Vector2f pos);
  void close() { m_open = false; }

  // Set when the user clicks "Draw on Canvas".
  // main.cpp should check this after render(), then clear it.
  bool drawOnCanvasRequested = false;
  sf::Vector2f getCreatePos() const { return m_createPos; }

private:
  void resetDefaults();
  void reinitEdges();

  // Build & configure figure for Circle or template slots
  std::unique_ptr<core::Figure> createConfiguredFigure(
      const std::vector<std::unique_ptr<core::Figure>>& userRegistry);

  // Build a PolylineFigure from the segment list
  std::unique_ptr<core::Figure> buildPolylineFromSegments();

  // Save built polyline to figures/ and register it
  void savePolylineAsTemplate(
      std::vector<Toolbar::CustomTool>& customTools,
      std::vector<std::unique_ptr<core::Figure>>& userRegistry);

  // ── Basic state ────────────────────────────────────────────────────────────
  bool m_open = false;
  sf::Vector2f m_createPos;

  // Dropdown index:  0=Circle  1=New Polyline  2+=templates
  int m_figureType = 0;
  char m_customName[256] = "";

  // ── Circle ─────────────────────────────────────────────────────────────────
  float m_radiusX = 50.f;
  float m_radiusY = 50.f;
  float m_fillColor[4] = {0.6f, 0.6f, 0.6f, 1.0f};

  struct TempEdge {
    float length = 100.f;
    float width  = 2.0f;
    float color[4] = {0.f, 0.f, 0.f, 1.f};
  };
  std::vector<TempEdge> m_edges;

  // ── New Polyline builder ───────────────────────────────────────────────────
  struct PolySegment {
    float length   = 100.f;
    float angleDeg = 0.f;
  };

  std::vector<PolySegment> m_polySegments;
  float m_polyEdgeWidth     = 2.f;
  float m_polyEdgeColor[4]  = {0.f, 0.f, 0.f, 1.f};
  float m_polyFillColor[4]  = {0.6f, 0.6f, 0.6f, 1.0f};
  char  m_polyTemplateName[256] = "my_figure";
  std::string m_polyStatusMsg;
};

} // namespace ui
