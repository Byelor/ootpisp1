#include "Scene.hpp"
#include "CompositeFigure.hpp"
#include "utils/GeometryUtils.hpp"

namespace core {

void Scene::observeExistingIdsRecursive(const Figure* fig) {
  if (!fig) return;
  if (fig->id != 0) {
    m_nextId = std::max(m_nextId, fig->id + 1);
  }
  if (auto cf = dynamic_cast<const CompositeFigure*>(fig)) {
    for (const auto& child : cf->children) {
      observeExistingIdsRecursive(child.figure.get());
    }
  }
}

void Scene::assignIdsRecursive(Figure* fig) {
  if (!fig) return;
  if (fig->id == 0) {
    fig->id = m_nextId++;
  } else {
    m_nextId = std::max(m_nextId, fig->id + 1);
  }
  if (auto cf = dynamic_cast<CompositeFigure*>(fig)) {
    for (auto& child : cf->children) {
      assignIdsRecursive(child.figure.get());
    }
  }
}

void Scene::addFigure(std::unique_ptr<Figure> fig) {
  observeExistingIdsRecursive(fig.get());
  assignIdsRecursive(fig.get());
  m_figures.add(std::move(fig));
}

std::unique_ptr<Figure> Scene::extractFigure(Figure *fig) {
  return m_figures.extract(fig);
}

bool Scene::removeFigure(Figure *fig) {
  return m_figures.remove(fig);
}

bool Scene::insertFigure(std::unique_ptr<Figure> fig, int index) {
  observeExistingIdsRecursive(fig.get());
  assignIdsRecursive(fig.get());
  return m_figures.insert(std::move(fig), index);
}

bool Scene::moveFigure(int fromIdx, int toIdx) {
  return m_figures.moveItem(fromIdx, toIdx);
}

Figure *Scene::hitTest(sf::Vector2f point) const {
  return m_figures.hitTest(point);
}

void Scene::drawAll(sf::RenderTarget &target, float markerScale) const {
  m_figures.drawAll(target, markerScale);

  // Draw bounding box if a figure is selected
  if (m_selectedFigure) {
    sf::FloatRect bounds = m_selectedFigure->getBoundingBox();
    sf::RectangleShape bbox(sf::Vector2f(bounds.width, bounds.height));
    bbox.setPosition(bounds.left, bounds.top);
    bbox.setFillColor(sf::Color::Transparent);
    bbox.setOutlineColor(sf::Color(0, 120, 215)); // Windows Blue
    bbox.setOutlineThickness(1.f * markerScale);
    target.draw(bbox);

    // Draw corner markers
    const float markerSize = 6.f * markerScale;
    sf::RectangleShape marker(sf::Vector2f(markerSize, markerSize));
    marker.setFillColor(sf::Color::White);
    marker.setOutlineColor(sf::Color(0, 120, 215));
    marker.setOutlineThickness(1.f * markerScale);
    marker.setOrigin(markerSize / 2.f, markerSize / 2.f);

    // Top-Left
    marker.setPosition(bounds.left, bounds.top);
    target.draw(marker);
    // Top-Right
    marker.setPosition(bounds.left + bounds.width, bounds.top);
    target.draw(marker);
    // Bottom-Right
    marker.setPosition(bounds.left + bounds.width, bounds.top + bounds.height);
    target.draw(marker);
    // Bottom-Left
    marker.setPosition(bounds.left, bounds.top + bounds.height);
    target.draw(marker);
  }
}

void Scene::setCustomOrigin(sf::Vector2f newOriginWorld) {
  for (int i = 0; i < m_figures.count(); ++i) {
    auto figure = m_figures.get(i);
    if (!customOriginActive) {
      figure->parentOrigin = newOriginWorld;
      figure->anchor -= newOriginWorld;
    } else {
      sf::Vector2f delta = newOriginWorld - customOriginPos;
      figure->parentOrigin = newOriginWorld;
      figure->anchor -= delta;
    }
  }
  customOriginPos = newOriginWorld;
  customOriginActive = true;
}

void Scene::resetCustomOrigin() {
  if (!customOriginActive)
    return;
  for (int i = 0; i < m_figures.count(); ++i) {
    auto figure = m_figures.get(i);
    figure->anchor += customOriginPos;
    figure->parentOrigin = sf::Vector2f(0.f, 0.f);
  }
  customOriginActive = false;
}

} // namespace core
