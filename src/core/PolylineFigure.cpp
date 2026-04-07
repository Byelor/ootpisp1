#include "PolylineFigure.hpp"
#include "MathUtils.hpp"
#include <cmath>

namespace core {

static float polygonSignedArea(const std::vector<sf::Vector2f>& verts) {
    float area = 0.f;
    int n = static_cast<int>(verts.size());
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += verts[i].x * verts[j].y - verts[j].x * verts[i].y;
    }
    return area;
}

PolylineFigure::PolylineFigure(std::vector<sf::Vector2f> vertices, std::string name)
    : figureName(std::move(name)) {
    m_vertices = std::move(vertices);
    edges.resize(m_vertices.size());
}

void PolylineFigure::setSideLengths(const std::vector<float>& lengths) {
    bool anyLockedAngle = false;
    const bool hasAngles = (static_cast<int>(lockedAngles.size()) ==
                            static_cast<int>(m_vertices.size()) &&
                            static_cast<int>(lockedAngleValues.size()) ==
                            static_cast<int>(m_vertices.size()));
    if (hasAngles)
        for (bool b : lockedAngles) if (b) { anyLockedAngle = true; break; }

    applyGenericSideLengths(lengths);

    if (!anyLockedAngle || !hasAngles) return;

    const int n = static_cast<int>(m_vertices.size());
    for (int pass = 0; pass < 5; ++pass) {
        if (pass > 0) {
            for (int i = 0; i < n && i < static_cast<int>(lengths.size()); ++i) {
                int j = (i + 1) % n;
                sf::Vector2f dir = m_vertices[j] - m_vertices[i];
                float dist = math::length(dir);
                if (dist < 1e-6f) continue;
                float err = (dist - lengths[i]) / dist;
                sf::Vector2f c = dir * (0.25f * err);
                m_vertices[i] += c;
                m_vertices[j] -= c;
            }
        }
        for (int i = 0; i < n; ++i) {
            if (!lockedAngles[i]) continue;
            setVertexAngle(i, lockedAngleValues[i]);
        }
    }
}

const char* PolylineFigure::getSideName(int idx) const {
    static thread_local std::string buf;
    buf = "Side " + std::to_string(idx);
    return buf.c_str();
}

std::unique_ptr<Figure> PolylineFigure::clone() const {
    auto copy = std::make_unique<PolylineFigure>();
    copy->figureName       = figureName;
    copy->anchor           = anchor;
    copy->parentOrigin     = parentOrigin;
    copy->fillColor        = fillColor;
    copy->rotationAngle    = rotationAngle;
    copy->scale            = scale;
    copy->edges            = edges;
    copy->lockedSides      = lockedSides;
    copy->lockedLengths    = lockedLengths;
    copy->lockedAngles     = lockedAngles;
    copy->lockedAngleValues = lockedAngleValues;
    copy->m_vertices       = m_vertices;
    return copy;
}

float PolylineFigure::getVertexAngle(int vertIdx) const {
    int n = static_cast<int>(m_vertices.size());
    if (n < 3 || vertIdx < 0 || vertIdx >= n) return 0.f;
    int prev = (vertIdx + n - 1) % n;
    int next = (vertIdx + 1) % n;

    sf::Vector2f vIn  = m_vertices[prev] - m_vertices[vertIdx];
    sf::Vector2f vOut = m_vertices[next] - m_vertices[vertIdx];

    float lenIn  = math::length(vIn);
    float lenOut = math::length(vOut);
    if (lenIn < 1e-6f || lenOut < 1e-6f) return 0.f;

    float cosA = (vIn.x * vOut.x + vIn.y * vOut.y) / (lenIn * lenOut);
    cosA = std::max(-1.f, std::min(1.f, cosA));
    float angle = std::acos(cosA) * 180.f / math::PI; // 0..180

   
    float cross = vIn.x * vOut.y - vIn.y * vOut.x;
    float orientation = polygonSignedArea(m_vertices) >= 0.f ? 1.f : -1.f;
    if (orientation * cross > 0.f) {
        angle = 360.f - angle;
    }
    return angle;
}

void PolylineFigure::setVertexAngle(int vertIdx, float angleDeg) {
    int n = static_cast<int>(m_vertices.size());
    if (n < 3 || vertIdx < 0 || vertIdx >= n) return;
    int prev = (vertIdx + n - 1) % n;
    int next = (vertIdx + 1) % n;

    sf::Vector2f vIn  = m_vertices[prev] - m_vertices[vertIdx];
    sf::Vector2f vOut = m_vertices[next] - m_vertices[vertIdx];

    float lenOut = math::length(vOut);
    if (lenOut < 1e-6f) return;

    float inAngle = std::atan2(vIn.y, vIn.x);
    float orientation = polygonSignedArea(m_vertices) >= 0.f ? 1.f : -1.f;

    float targetOutAngle = inAngle - orientation * angleDeg * math::DEG_TO_RAD;

    sf::Vector2f newVOut(std::cos(targetOutAngle) * lenOut,
                         std::sin(targetOutAngle) * lenOut);
    sf::Vector2f delta = (m_vertices[vertIdx] + newVOut) - m_vertices[next];
    m_vertices[next] = m_vertices[vertIdx] + newVOut;

    for (int j = (next + 1) % n; j != vertIdx; j = (j + 1) % n) {
        m_vertices[j] += delta;
    }
}

void PolylineFigure::serialize(std::ostream& out, int indent) const {
    Figure::serialize(out, indent);
    std::string pad(indent, ' ');
    out << pad << "name " << figureName << "\n";
    auto verts = getVertices(); // save current relative vertices
    out << pad << "vertices_base " << verts.size() << "\n";
    for (size_t i = 0; i < verts.size(); ++i) {
        out << pad << "  vertex " << i << " " << verts[i].x << " " << verts[i].y << "\n";
    }
}

bool PolylineFigure::deserialize(const std::string& prop, std::istream& in) {
    if (prop == "name") {
        in >> std::ws;
        std::getline(in, figureName);
        return true;
    } else if (prop == "vertices" || prop == "vertices_base") {
        size_t count;
        in >> count;
        m_vertices.resize(count);
        for (size_t i = 0; i < count; ++i) {
            std::string dummy;
            size_t idx;
            float px, py;
            in >> dummy >> idx >> px >> py;
            if (idx < count) {
                m_vertices[idx] = {px, py};
            }
        }
        return true;
    }
    
    // Pass everything else to base class
    return Figure::deserialize(prop, in);
}

} // namespace core
