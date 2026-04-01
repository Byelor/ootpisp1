#include "CompositeFigure.hpp"
#include "Scene.hpp"
#include "MathUtils.hpp"
#include "utils/GeometryUtils.hpp"
#include <algorithm>
#include <cmath>

namespace core {

CompositeFigure::CompositeFigure(std::vector<sf::Vector2f> vertices, std::string name)
    : figureName(std::move(name)) {
    m_vertices = std::move(vertices);
    edges.resize(m_vertices.size());
}

std::unique_ptr<CompositeFigure> CompositeFigure::mergeFromScene(
    const std::vector<Figure*>& figs,
    Scene& scene,
    const std::string& name
) {
    if (figs.size() < 2) return nullptr;

    // Вычислить центр группы
    sf::Vector2f center = {0.f, 0.f};
    for (auto* f : figs) center += f->getAbsoluteAnchor();
    center /= static_cast<float>(figs.size());

    auto compound = std::make_unique<CompositeFigure>();
    compound->figureName = name;
    compound->anchor     = center;

    for (auto* f : figs) {
        Child c;
        // Сохранить относительное смещение
        sf::Vector2f relOffset = f->getAbsoluteAnchor() - center;
        auto ptr = scene.extractFigure(f);
        if (ptr) {
            ptr->parentFigure = compound.get();
            ptr->anchor       = relOffset;
            ptr->parentOrigin = {0.f, 0.f};
            c.figure = std::move(ptr);
            compound->children.push_back(std::move(c));
        }
    }
    return compound;
}

// ─── Overrides 

void CompositeFigure::draw(sf::RenderTarget& target) const {
    if (!m_vertices.empty()) {
        Figure::draw(target);
    }
    for (const auto& child : children) {
        child.figure->draw(target);
    }
}

std::unique_ptr<Figure> CompositeFigure::extractChild(Figure* childPtr) {
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->figure.get() == childPtr) {
            auto ptr = std::move(it->figure);
            children.erase(it);
            // Возвращаем абсолютное положение
            sf::Vector2f absAnchor = getAbsoluteAnchor() + ptr->anchor;
            ptr->parentFigure = nullptr;
            ptr->anchor       = absAnchor;
            ptr->parentOrigin = {0.f, 0.f};
            return ptr;
        }
    }
    return nullptr;
}

void CompositeFigure::insertChild(std::unique_ptr<Figure> childFigure, int index, sf::Vector2f localOffset, float localRotation) {
    if (!childFigure) return;
    if (index < 0) index = 0;
    if (index > children.size()) index = children.size();

    Child c;
    c.figure = std::move(childFigure);
    c.figure->parentFigure = this;
    c.figure->anchor = localOffset;
    c.figure->rotationAngle = localRotation;

    children.insert(children.begin() + index, std::move(c));
}

bool CompositeFigure::moveChild(int fromIdx, int toIdx) {
    if (fromIdx < 0 || fromIdx >= children.size() || toIdx < 0 || toIdx >= children.size()) return false;
    if (fromIdx == toIdx) return true;

    auto temp = std::move(children[fromIdx]);
    if (fromIdx < toIdx) {
        for (int i = fromIdx; i < toIdx; ++i) {
            children[i] = std::move(children[i + 1]);
        }
    } else {
        for (int i = fromIdx; i > toIdx; --i) {
            children[i] = std::move(children[i - 1]);
        }
    }
    children[toIdx] = std::move(temp);
    return true;
}

bool CompositeFigure::contains(sf::Vector2f point) const {
    if (!m_vertices.empty() && Figure::contains(point)) {
        return true;
    }
    for (const auto& child : children) {
        if (child.figure->contains(point)) return true;
    }
    return false;
}

Figure* CompositeFigure::hitTestChild(sf::Vector2f point) const {
    // Traverse in reverse to pick top-most children first
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        const auto& child = *it;
        
        if (auto childComp = dynamic_cast<CompositeFigure*>(child.figure.get())) {
            Figure* subHit = childComp->hitTestChild(point);
            if (subHit) return subHit;
        }
        
        // Use basic contains if it's not a composite, or composite hitTestChild failed
        if (child.figure->contains(point)) {
            return child.figure.get();
        }
    }
    return nullptr;
}

bool CompositeFigure::snapChildToSiblings(Figure* child) {
    if (children.size() < 2) return true;

    sf::FloatRect childBounds = child->getBoundingBox();
    const float kTouchThreshold = 8.f; // pixels (world space)

    float bestSnapDist = 1e18f;
    sf::Vector2f bestDeltaWorld = {0.f, 0.f};
    bool alreadyTouching = false;

    for (const auto& c : children) {
        if (c.figure.get() == child) continue;
        sf::FloatRect sibBounds = c.figure->getBoundingBox();

        bool overlapX = childBounds.left < sibBounds.left + sibBounds.width &&
                        childBounds.left + childBounds.width > sibBounds.left;
        bool overlapY = childBounds.top  < sibBounds.top  + sibBounds.height &&
                        childBounds.top  + childBounds.height > sibBounds.top;

        // Gap in each direction (positive = child needs to move that way)
        float gapRight  =  sibBounds.left  - (childBounds.left + childBounds.width);
        float gapLeft   = -(childBounds.left - (sibBounds.left + sibBounds.width));
        float gapBottom =  sibBounds.top   - (childBounds.top  + childBounds.height);
        float gapTop    = -(childBounds.top  - (sibBounds.top  + sibBounds.height));

        // Already touching or overlapping
        if ((overlapX || std::abs(gapRight) < kTouchThreshold || std::abs(gapLeft) < kTouchThreshold) &&
            (overlapY || std::abs(gapBottom) < kTouchThreshold || std::abs(gapTop) < kTouchThreshold)) {
            alreadyTouching = true;
            break;
        }

        // Try all 4 edge-snap directions and pick the closest
        auto trySnap = [&](sf::Vector2f worldDelta) {
            float dist = std::hypot(worldDelta.x, worldDelta.y);
            if (dist < bestSnapDist) {
                bestSnapDist = dist;
                bestDeltaWorld = worldDelta;
            }
        };

        if (overlapY || std::abs(std::min(gapRight, gapLeft)) < 80.f) {
            trySnap({gapRight, 0.f});
            trySnap({-gapLeft, 0.f});
        }
        if (overlapX || std::abs(std::min(gapBottom, gapTop)) < 80.f) {
            trySnap({0.f, gapBottom});
            trySnap({0.f, -gapTop});
        }
    }

    if (alreadyTouching) return true;
    if (bestSnapDist >= 1e17f) return false;

    // Convert world-space delta to parent-local anchor delta
    // Parent may be rotated/scaled — invert the parent transform
    sf::Vector2f localDelta = bestDeltaWorld;
    if (child->parentFigure) {
        float parentRot = child->parentFigure->getAbsoluteRotation();
        sf::Vector2f parentScale = child->parentFigure->getAbsoluteScale();
        float rad = -parentRot * math::DEG_TO_RAD;
        float rx = localDelta.x * std::cos(rad) - localDelta.y * std::sin(rad);
        float ry = localDelta.x * std::sin(rad) + localDelta.y * std::cos(rad);
        localDelta = {rx / parentScale.x, ry / parentScale.y};
    }

    child->anchor += localDelta;
    return false;
}

const std::vector<sf::Vector2f>& CompositeFigure::getVertices() const {
    return m_vertices;
}

sf::FloatRect CompositeFigure::getBoundingBox() const {
    sf::FloatRect bounds;
    bool first = true;

    if (!m_vertices.empty()) {
        bounds = Figure::getBoundingBox();
        first = false;
    }

    for (const auto& child : children) {
        sf::FloatRect childBounds = child.figure->getBoundingBox();
        if (first) {
            bounds = childBounds;
            first = false;
        } else {
            float left = std::min(bounds.left, childBounds.left);
            float top = std::min(bounds.top, childBounds.top);
            float right = std::max(bounds.left + bounds.width, childBounds.left + childBounds.width);
            float bottom = std::max(bounds.top + bounds.height, childBounds.top + childBounds.height);
            bounds = sf::FloatRect(left, top, right - left, bottom - top);
        }
    }
    return bounds;
}

sf::FloatRect CompositeFigure::getLocalBoundingBox() const {
    // We want the bounding box in the local coordinate space of THIS figure.
    // That is, relative to our anchor, with our rotation and scale INVERTED.
    
    sf::FloatRect absoluteBounds = getBoundingBox();
    
    sf::Vector2f absAnchor = getAbsoluteAnchor();
    float absRot = getAbsoluteRotation();
    sf::Vector2f absScale = getAbsoluteScale();
    
    auto toLocal = [&](sf::Vector2f worldP) -> sf::Vector2f {
        sf::Vector2f rel = worldP - absAnchor;
        // Unrotate
        float rad = -absRot * math::DEG_TO_RAD;
        sf::Vector2f unrot(rel.x * std::cos(rad) - rel.y * std::sin(rad),
                            rel.x * std::sin(rad) + rel.y * std::cos(rad));
        // Unscale
        return { (absScale.x != 0.f) ? unrot.x / absScale.x : 0.f,
                 (absScale.y != 0.f) ? unrot.y / absScale.y : 0.f };
    };
    
    // Project the 4 corners of the absolute bounding box into local space
    sf::Vector2f corners[4] = {
        {absoluteBounds.left, absoluteBounds.top},
        {absoluteBounds.left + absoluteBounds.width, absoluteBounds.top},
        {absoluteBounds.left, absoluteBounds.top + absoluteBounds.height},
        {absoluteBounds.left + absoluteBounds.width, absoluteBounds.top + absoluteBounds.height}
    };
    
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    for (int i = 0; i < 4; ++i) {
        sf::Vector2f lp = toLocal(corners[i]);
        if (i == 0) {
            minX = maxX = lp.x;
            minY = maxY = lp.y;
        } else {
            minX = std::min(minX, lp.x);
            minY = std::min(minY, lp.y);
            maxX = std::max(maxX, lp.x);
            maxY = std::max(maxY, lp.y);
        }
    }
    
    return sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
}

std::string CompositeFigure::typeName() const {
    return "composite";
}

std::unique_ptr<Figure> CompositeFigure::clone() const {
    auto copy = std::make_unique<CompositeFigure>();
    copy->figureName = figureName;
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;

    for (const auto& child : children) {
        Child c;
        c.figure = child.figure->clone();
        c.figure->parentFigure = copy.get();
        copy->children.push_back(std::move(c));
    }
    return copy;
}

void CompositeFigure::applyScale() {
    Figure::applyScale();
    for (auto& child : children) {
        child.figure->anchor.x *= scale.x;
        child.figure->anchor.y *= scale.y;
    }
    scale = {1.f, 1.f};
}

void CompositeFigure::resetAnchor() {
    sf::FloatRect bounds = getLocalBoundingBox();
    sf::Vector2f localCenter(bounds.left + bounds.width / 2.0f,
        bounds.top + bounds.height / 2.0f);

    if (std::abs(localCenter.x) < 0.001f && std::abs(localCenter.y) < 0.001f) {
        return;
    }

    sf::Vector2f rotated = math::rotate(
        sf::Vector2f(localCenter.x * scale.x, localCenter.y * scale.y),
        rotationAngle * math::DEG_TO_RAD);

    anchor += rotated;

    for (auto& v : m_vertices) {
        v -= localCenter;
    }
    
    for (auto& child : children) {
        child.figure->anchor -= localCenter;
    }
}

void CompositeFigure::setAnchorKeepAbsolute(sf::Vector2f newAnchor) {
    if (math::length(newAnchor - anchor) < 0.001f) {
        return;
    }

    sf::Vector2f oldAnchor = anchor;
    Figure::setAnchorKeepAbsolute(newAnchor);

    sf::Vector2f deltaA = newAnchor - oldAnchor;
    sf::Vector2f rotated = math::rotate(deltaA, -rotationAngle * math::DEG_TO_RAD);

    float vx = (scale.x != 0.f) ? (rotated.x / scale.x) : 0.f;
    float vy = (scale.y != 0.f) ? (rotated.y / scale.y) : 0.f;

    for (auto& child : children) {
        child.figure->anchor.x -= vx;
        child.figure->anchor.y -= vy;
    }
}

} // namespace core
