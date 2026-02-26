#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

namespace core {

struct Edge {
  float width = 1.0f;
  sf::Color color = sf::Color::Black;
};

class Figure {
public:
  virtual ~Figure() = default;

  // Anchor point (absolute coordinates on the canvas)
  sf::Vector2f anchor{0.f, 0.f};

  // Fill color
  sf::Color fillColor = sf::Color::White;

  // Rotation angle in degrees
  float rotationAngle = 0.f;

  // Scale multipliers
  sf::Vector2f scale{1.f, 1.f};

  // Line styles for each edge (could be simpler and just one style for the
  // whole outline, but plan says per edge)
  std::vector<Edge> edges;

  // Get the bounding box of the figure based on its absolute vertices
  sf::FloatRect getBoundingBox() const;

  // Get the unscaled local bounding box based on relative vertices
  sf::FloatRect getLocalBoundingBox() const;

  // Get relative vertices (offset from anchor)
  virtual const std::vector<sf::Vector2f> &getVertices() const {
    return m_vertices;
  }
  virtual std::vector<sf::Vector2f> &getVerticesMutable() { return m_vertices; }

  // Get absolute vertex position including anchor and rotation
  sf::Vector2f getAbsoluteVertex(sf::Vector2f relative) const;

  // Draw the figure (fill and outline)
  virtual void draw(sf::RenderTarget &target) const;

  // Check if the figure contains a given absolute point
  virtual bool contains(sf::Vector2f point) const;

  // Move the figure relative to its current anchor
  void move(sf::Vector2f delta);

protected:
  // Relative vertices for this figure
  std::vector<sf::Vector2f> m_vertices;
};

} // namespace core
