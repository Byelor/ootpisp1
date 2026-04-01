#pragma once

#include "Figure.hpp"
#include <string>
#include <vector>
#include <memory>

namespace core {

class Scene; // Forward declaration

class CompositeFigure : public Figure {
public:
    std::string figureName;
    bool isSolidGroup = false; // subfigures must touch; can slide but not separate

    struct Child {
        std::unique_ptr<Figure> figure;
    };

    std::vector<Child> children;

    // Creates a CompositeFigure from a list of Figure* on the scene
    static std::unique_ptr<CompositeFigure> mergeFromScene(
        const std::vector<Figure*>& figs,
        Scene& scene,
        const std::string& name
    );

    std::unique_ptr<Figure> extractChild(Figure* childPtr);
    void insertChild(std::unique_ptr<Figure> childFigure, int index, sf::Vector2f localOffset, float localRotation = 0.f);
    bool moveChild(int fromIdx, int toIdx);

    Figure* hitTestChild(sf::Vector2f point) const;

    // For solid groups: snap the given child so it touches at least one sibling.
    // Returns true if a snap was applied.
    bool snapChildToSiblings(Figure* child);

    CompositeFigure() = default;
    explicit CompositeFigure(std::vector<sf::Vector2f> vertices, std::string name = "Custom");

    // Overrides
    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;
    
    // Bounding box override to account for children
    sf::FloatRect getBoundingBox() const override;
    sf::FloatRect getLocalBoundingBox() const override;
    const std::vector<sf::Vector2f>& getVertices() const override;

    std::string typeName() const override;
    std::unique_ptr<Figure> clone() const override;
    void applyScale() override;

    void resetAnchor() override;
    void setAnchorKeepAbsolute(sf::Vector2f newAnchor) override;

private:
    mutable std::vector<sf::Vector2f> m_cachedVerts;
};

} // namespace core
