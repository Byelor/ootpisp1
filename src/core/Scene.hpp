#pragma once

#include "Figure.hpp"
#include <memory>
#include "SceneArray.hpp"

namespace core {

class Scene {
public:
  void addFigure(std::unique_ptr<Figure> fig);
  bool insertFigure(std::unique_ptr<Figure> fig, int index);
  bool moveFigure(int fromIdx, int toIdx);

  // Removes the given figure from the scene. Returns true if removed.
  bool removeFigure(Figure *fig);

  // Returns the top-most figure at the given absolute point, or nullptr if none
  Figure *hitTest(sf::Vector2f point) const;

  // Draw all figures
  void drawAll(sf::RenderTarget &target, float markerScale = 1.0f) const;

  int figureCount() const { return m_figures.count(); }
  Figure *getFigure(int idx) const { return m_figures.get(idx); }
  std::unique_ptr<Figure> extractFigure(Figure *fig);

  // Selected figure
  void setSelectedFigure(Figure *fig) { m_selectedFigure = fig; }
  Figure *getSelectedFigure() const { return m_selectedFigure; }

  // World Origin properties
  bool customOriginActive = false;
  sf::Vector2f customOriginPos{0.f, 0.f};

  void setCustomOrigin(sf::Vector2f newOriginWorld);
  void resetCustomOrigin();

private:
  SceneArray m_figures;
  Figure *m_selectedFigure = nullptr;
};

} // namespace core
