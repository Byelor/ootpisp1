#include "PolylineFigure.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

PolylineFigure::PolylineFigure(std::vector<sf::Vector2f> vertices, std::string name)
    : figureName(std::move(name)) {
    m_vertices = std::move(vertices);
    edges.resize(m_vertices.size());
}

void PolylineFigure::setSideLengths(const std::vector<float>& lengths) {
    applyGenericSideLengths(lengths);
}

const char* PolylineFigure::getSideName(int idx) const {
    static thread_local std::string buf;
    buf = "Side " + std::to_string(idx);
    return buf.c_str();
}

std::unique_ptr<Figure> PolylineFigure::clone() const {
    auto copy = std::make_unique<PolylineFigure>();
    copy->figureName    = figureName;
    copy->anchor        = anchor;
    copy->parentOrigin  = parentOrigin;
    copy->fillColor     = fillColor;
    copy->rotationAngle = rotationAngle;
    copy->scale         = scale;
    copy->edges         = edges;
    copy->lockedSides   = lockedSides;
    copy->lockedLengths = lockedLengths;
    copy->m_vertices    = m_vertices;
    return copy;
}

void PolylineFigure::setEdgeAngle(int edgeIdx, float angleDeg) {
    int n = static_cast<int>(m_vertices.size());
    if (n < 2 || edgeIdx < 0 || edgeIdx >= n) return;
    int i    = edgeIdx;
    int next = (i + 1) % n;
    float len = math::length(m_vertices[next] - m_vertices[i]);
    float rad = angleDeg * math::DEG_TO_RAD;
    
    // Сдвигаем только следующую вершину
    sf::Vector2f newNext = m_vertices[i] + sf::Vector2f(std::cos(rad), std::sin(rad)) * len;
    sf::Vector2f delta   = newNext - m_vertices[next];
    m_vertices[next]     = newNext;
    
    // Сдвигаем все последующие вершины на тот же delta (кроме первой)
    for (int j = next + 1; j < n; ++j) {
        if (j != i) {
            m_vertices[j] += delta;
        }
    }
}

float PolylineFigure::getEdgeAngle(int edgeIdx) const {
    int n = static_cast<int>(m_vertices.size());
    if (n < 2 || edgeIdx < 0 || edgeIdx >= n) return 0.f;
    int next = (edgeIdx + 1) % n;
    
    sf::Vector2f dir = m_vertices[next] - m_vertices[edgeIdx];
    float angleRad = std::atan2(dir.y, dir.x);
    float angleDeg = angleRad * 180.f / math::PI;
    
    while (angleDeg <= -180.f) angleDeg += 360.f;
    while (angleDeg > 180.f) angleDeg -= 360.f;
    
    return angleDeg;
}

} // namespace core
