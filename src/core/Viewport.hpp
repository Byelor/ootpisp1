#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>

namespace core {

class Viewport {
public:
  sf::Vector2f worldOrigin{0.f, 0.f};
  float zoom = 1.f;

  sf::Vector2f worldToScreen(sf::Vector2f world) const {
    return worldOrigin + world * zoom;
  }

  sf::Vector2f screenToWorld(sf::Vector2f screen) const {
    return (screen - worldOrigin) / zoom;
  }

  void zoomAt(sf::Vector2f screenPoint, float factor) {
    sf::Vector2f worldPoint = screenToWorld(screenPoint);
    zoom *= factor;
    zoom = std::clamp(zoom, 0.05f, 50.f);
    worldOrigin = screenPoint - worldPoint * zoom;
  }

  // Generates an sf::View that corresponds to this viewport
  sf::View getView(sf::Vector2f windowSize) const {
    sf::View view;
    sf::Vector2f worldCenter =
        screenToWorld(sf::Vector2f(windowSize.x / 2.f, windowSize.y / 2.f));
    view.setCenter(worldCenter);
    view.setSize(windowSize.x / zoom, windowSize.y / zoom);
    return view;
  }
};

} // namespace core
