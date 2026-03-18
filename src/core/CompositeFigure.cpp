#include "CompositeFigure.hpp"
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

// ─── Factory Methods ─────────────────────────────────────────────────────────

std::unique_ptr<CompositeFigure> CompositeFigure::createRectangle(float width, float height) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Rectangle";
    fig->preset = Preset::Rectangle;
    fig->edges.resize(4);
    fig->m_vertices = {
        {-width / 2.f, -height / 2.f},
        {width / 2.f, -height / 2.f},
        {width / 2.f, height / 2.f},
        {-width / 2.f, height / 2.f}
    };
    fig->m_cachedWidth = width;
    fig->m_cachedHeight = height;
    return fig;
}

std::unique_ptr<CompositeFigure> CompositeFigure::createTriangle(float base, float height) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Triangle";
    fig->preset = Preset::Triangle;
    fig->edges.resize(3);
    fig->m_vertices = {
        {0.f, -height / 2.f},
        {base / 2.f, height / 2.f},
        {-base / 2.f, height / 2.f}
    };
    fig->m_cachedWidth = base;
    fig->m_cachedHeight = height;
    return fig;
}

std::unique_ptr<CompositeFigure> CompositeFigure::createHexagon(float width, float height) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Hexagon";
    fig->preset = Preset::Hexagon;
    fig->edges.resize(6);
    fig->m_vertices.resize(6);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 60.f * math::PI / 180.f;
        fig->m_vertices[i] = { (width / 2.f) * std::cos(angle), (height / 2.f) * std::sin(angle) };
    }
    fig->m_cachedWidth = width;
    fig->m_cachedHeight = height;
    return fig;
}

std::unique_ptr<CompositeFigure> CompositeFigure::createRhombus(float width, float height) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Rhombus";
    fig->preset = Preset::Rhombus;
    fig->edges.resize(4);
    fig->m_vertices = {
        {0.f, -height / 2.f},
        {width / 2.f, 0.f},
        {0.f, height / 2.f},
        {-width / 2.f, 0.f}
    };
    fig->m_cachedWidth = width;
    fig->m_cachedHeight = height;
    return fig;
}

std::unique_ptr<CompositeFigure> CompositeFigure::createTrapezoid(float topWidth, float bottomWidth, float height) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Trapezoid";
    fig->preset = Preset::Trapezoid;
    fig->edges.resize(4);
    fig->m_vertices = {
        {-topWidth / 2.f, -height / 2.f},
        {topWidth / 2.f, -height / 2.f},
        {bottomWidth / 2.f, height / 2.f},
        {-bottomWidth / 2.f, height / 2.f}
    };
    fig->m_cachedTopWidth = topWidth;
    fig->m_cachedBottomWidth = bottomWidth;
    fig->m_cachedHeight = height;
    return fig;
}

std::unique_ptr<CompositeFigure> CompositeFigure::createCircle(float radiusX, float radiusY) {
    auto fig = std::make_unique<CompositeFigure>();
    fig->figureName = "Circle";
    fig->preset = Preset::Circle;
    int segments = 60;
    fig->edges.resize(1); // Treated as single uniform edge mostly
    fig->m_vertices.resize(segments);
    for (int i = 0; i < segments; ++i) {
        float angle = i * 2.f * math::PI / segments;
        fig->m_vertices[i] = {radiusX * std::cos(angle), radiusY * std::sin(angle)};
    }
    fig->m_cachedWidth = radiusX;
    fig->m_cachedHeight = radiusY;
    return fig;
}

// ─── Overrides ───────────────────────────────────────────────────────────────

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
            ptr->parentFigure = nullptr;
            children.erase(it);
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
    if (children.empty()) return m_vertices;

    m_cachedVerts = m_vertices;
    for (const auto& child : children) {
        const auto& cverts = child.figure->getVertices();
        for (const auto& cv : cverts) {
            sf::Vector2f cv_scaled(cv.x * child.figure->scale.x, cv.y * child.figure->scale.y);
            sf::Vector2f cv_rot = math::rotate(cv_scaled, child.figure->rotationAngle * math::DEG_TO_RAD);
            
            sf::Vector2f offset = child.figure->anchor + cv_rot;
            m_cachedVerts.push_back(offset);
        }
    }
    return m_cachedVerts; // Returns a combined set of ALL vertices for hit test bounding boxes
}

bool CompositeFigure::hasSideLengths() const {
    return preset != Preset::Circle && m_vertices.size() > 2;
}

void CompositeFigure::setSideLengths(const std::vector<float>& lengths) {
    if (lengths.size() < m_vertices.size()) return;

    if (preset == Preset::Rectangle) {
        float top = std::max(1.f, lengths[0]);
        float right = std::max(1.f, lengths[1]);
        float bottom = std::max(1.f, lengths[2]);
        float left = std::max(1.f, lengths[3]);
        float w = top;
        float h = left;
        m_cachedWidth = w;
        m_cachedHeight = h;
        m_vertices = {
            {-w / 2.f, -h / 2.f},
            {w / 2.f, -h / 2.f},
            {w / 2.f, h / 2.f},
            {-w / 2.f, h / 2.f}
        };
    } else if (preset == Preset::Triangle) {
        float b = std::max(1.f, lengths[0]); 
        float a = std::max(1.f, lengths[1]); 
        float c = std::max(1.f, lengths[2]); 
        float sumBC = b + c, sumAC = a + c, sumAB = a + b;
        if (a >= sumBC) a = sumBC - 1.f;
        if (b >= sumAC) b = sumAC - 1.f;
        if (c >= sumAB) c = sumAB - 1.f;
        float x2 = (c * c + a * a - b * b) / (2.f * a);
        float y2sq = c * c - x2 * x2;
        float y2 = -std::sqrt(std::max(0.f, y2sq)); 
        float cx = (0.f + a + x2) / 3.f;
        float cy = (0.f + 0.f + y2) / 3.f;
        m_vertices = {
            {x2 - cx, y2 - cy},  
            {a - cx, 0.f - cy},  
            {0.f - cx, 0.f - cy} 
        };
        m_cachedWidth = a;
        m_cachedHeight = std::abs(y2);
    } else if (preset == Preset::Trapezoid) {
        float top = std::max(1.f, lengths[0]);
        float right = std::max(1.f, lengths[1]);
        float bottom = std::max(1.f, lengths[2]);
        float left = std::max(1.f, lengths[3]);
        float D = top - bottom;
        float dx_left, dx_right;
        if (std::abs(D) < 1e-4f) {
            dx_left = 0.f; dx_right = 0.f;
        } else {
            dx_left = (right * right - D * D - left * left) / (2.f * D);
            dx_right = dx_left + D;
        }
        float h = std::sqrt(std::max(1.f, left * left - dx_left * dx_left));
        float BLx = 0.f, BLy = 0.f;
        float BRx = bottom, BRy = 0.f;
        float TRx = BRx + dx_right, TRy = -h;
        float TLx = BLx + dx_left, TLy = -h;
        float cx = (BLx + BRx + TRx + TLx) / 4.f;
        float cy = (BLy + BRy + TRy + TLy) / 4.f;
        m_vertices = {
            {TLx - cx, TLy - cy},
            {TRx - cx, TRy - cy},
            {BRx - cx, BRy - cy},
            {BLx - cx, BLy - cy}
        };
        m_cachedTopWidth = top;
        m_cachedBottomWidth = bottom;
        m_cachedHeight = h;
    } else if (preset == Preset::Hexagon) {
        std::vector<float> curr_lengths = lengths;
        for (float& l : curr_lengths) l = std::max(1.f, l);
        applyGenericSideLengths(curr_lengths);
    } else if (preset == Preset::Rhombus) {
        std::vector<float> curr_lengths = lengths;
        for (float& l : curr_lengths) l = std::max(1.f, l);
        applyGenericSideLengths(curr_lengths);
    } else {
        // Polyline / Generic
        applyGenericSideLengths(lengths);
    }
}

const char* CompositeFigure::getSideName(int idx) const {
    if (preset == Preset::Rectangle) {
        static const char* n[] = {"Top", "Right", "Bottom", "Left"};
        return (idx >= 0 && idx < 4) ? n[idx] : "Side";
    } else if (preset == Preset::Triangle) {
        static const char* n[] = {"Right side", "Bottom", "Left side"};
        return (idx >= 0 && idx < 3) ? n[idx] : "Side";
    } else if (preset == Preset::Hexagon) {
        return "Side";
    } else if (preset == Preset::Rhombus) {
        static const char* n[] = {"Top-R", "Bottom-R", "Bottom-L", "Top-L"};
        return (idx >= 0 && idx < 4) ? n[idx] : "Side";
    } else if (preset == Preset::Trapezoid) {
        static const char* n[] = {"Top", "Right leg", "Bottom", "Left leg"};
        return (idx >= 0 && idx < 4) ? n[idx] : "Side";
    }
    return "Side";
}

std::string CompositeFigure::typeName() const {
    return preset != Preset::None ? figureName : "composite";
}

std::unique_ptr<Figure> CompositeFigure::clone() const {
    auto copy = std::make_unique<CompositeFigure>();
    copy->figureName = figureName;
    copy->preset = preset;
    copy->anchor = anchor;
    copy->parentOrigin = parentOrigin;
    copy->fillColor = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale = scale;
    copy->edges = edges;
    copy->lockedSides = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices = m_vertices;
    copy->m_cachedWidth = m_cachedWidth;
    copy->m_cachedHeight = m_cachedHeight;
    copy->m_cachedTopWidth = m_cachedTopWidth;
    copy->m_cachedBottomWidth = m_cachedBottomWidth;

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

bool CompositeFigure::hasUniformEdge() const {
    return preset == Preset::Circle;
}

// ─── Polyline Angles ─────────────────────────────────────────────────────────

void CompositeFigure::setEdgeAngle(int edgeIdx, float angleDeg) {
    if (m_vertices.size() < 3 || edgeIdx < 0 || edgeIdx >= static_cast<int>(m_vertices.size()))
        return;

    int n = m_vertices.size();
    int i = edgeIdx;
    int prev = (i - 1 + n) % n;
    int next = (i + 1) % n;

    sf::Vector2f v_prev = m_vertices[prev];
    sf::Vector2f v_curr = m_vertices[i];
    sf::Vector2f v_next = m_vertices[next];

    sf::Vector2f dir_prev_curr = v_curr - v_prev;
    float len_next = math::length(v_next - v_curr);

    float baseAngleRad = std::atan2(dir_prev_curr.y, dir_prev_curr.x);
    float newAngleRad = baseAngleRad + angleDeg * math::PI / 180.f;

    sf::Vector2f newDir(std::cos(newAngleRad), std::sin(newAngleRad));
    m_vertices[next] = v_curr + newDir * len_next;
}

float CompositeFigure::getEdgeAngle(int edgeIdx) const {
    if (m_vertices.size() < 3 || edgeIdx < 0 || edgeIdx >= static_cast<int>(m_vertices.size()))
        return 0.f;

    int n = m_vertices.size();
    int i = edgeIdx;
    int prev = (i - 1 + n) % n;
    int next = (i + 1) % n;

    sf::Vector2f dirIn = m_vertices[i] - m_vertices[prev];
    sf::Vector2f dirOut = m_vertices[next] - m_vertices[i];

    float angleIn = std::atan2(dirIn.y, dirIn.x);
    float angleOut = std::atan2(dirOut.y, dirOut.x);

    float diff = angleOut - angleIn;
    float currAngleDeg = diff * 180.f / math::PI;

    while (currAngleDeg <= -180.f) currAngleDeg += 360.f;
    while (currAngleDeg > 180.f) currAngleDeg -= 360.f;
    
    return currAngleDeg;
}

} // namespace core
