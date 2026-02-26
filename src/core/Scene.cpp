#include "Scene.hpp"

namespace core {

void Scene::addFigure(std::unique_ptr<Figure> fig) {
  if (fig) {
    m_figures.push_back(std::move(fig));
  }
}

bool Scene::removeFigure(Figure *fig) {
  for (auto it = m_figures.begin(); it != m_figures.end(); ++it) {
    if (it->get() == fig) {
      m_figures.erase(it);
      return true;
    }
  }
  return false;
}

Figure *Scene::hitTest(sf::Vector2f point) const {
  // Iterate backwards to check top-most figures first
  for (auto it = m_figures.rbegin(); it != m_figures.rend(); ++it) {
    if ((*it)->contains(point)) {
      return it->get();
    }
  }
  return nullptr;
}

void Scene::drawAll(sf::RenderTarget &target) const {
  for (const auto &fig : m_figures) {
    fig->draw(target);
  }

  // Draw bounding box if a figure is selected
  if (m_selectedFigure) {
    sf::FloatRect bounds = m_selectedFigure->getBoundingBox();
    sf::RectangleShape bbox(sf::Vector2f(bounds.width, bounds.height));
    bbox.setPosition(bounds.left, bounds.top);
    bbox.setFillColor(sf::Color::Transparent);
    bbox.setOutlineColor(sf::Color(0, 120, 215)); // Windows Blue
    bbox.setOutlineThickness(1.f);
    target.draw(bbox);

    // Draw corner markers
    const float markerSize = 6.f;
    sf::RectangleShape marker(sf::Vector2f(markerSize, markerSize));
    marker.setFillColor(sf::Color::White);
    marker.setOutlineColor(sf::Color(0, 120, 215));
    marker.setOutlineThickness(1.f);
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

} // namespace core
