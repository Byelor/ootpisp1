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

  // Anchor point (relative to parentOrigin)
  sf::Vector2f anchor{0.f, 0.f};

  // Parent origin offset
  sf::Vector2f parentOrigin{0.f, 0.f};

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

  // Reset the anchor to the center of the local bounding box without moving the
  // figure
  void resetAnchor();

  // Set a new anchor point while keeping the figure's absolute position
  // invariant
  void setAnchorKeepAbsolute(sf::Vector2f newAnchor);

  // Apply the current scale factor permanently to the figure's vertices
  // and reset the scale vector to (1, 1).
  void applyScale();

  // Return true if all edges should be styled identically in the UI
  virtual bool hasUniformEdge() const { return false; }

  // Return true if this figure supports per-side length editing
  virtual bool hasSideLengths() const { return false; }

  // Return the display name of side i (e.g. "Top", "Right leg")
  virtual const char *getSideName(int /*idx*/) const { return "Side"; }

  // Return the current side lengths (distance between consecutive vertices)
  virtual std::vector<float> getSideLengths() const;

  // Set new side lengths and recompute vertices to match.
  // Subclasses override this to implement their geometry solver.
  virtual void setSideLengths(const std::vector<float> & /*lengths*/) {}

protected:
  // Relative vertices for this figure
  std::vector<sf::Vector2f> m_vertices;
};

} // namespace core
