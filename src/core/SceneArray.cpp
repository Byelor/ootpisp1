#include "SceneArray.hpp"
#include <algorithm>

namespace core {

bool SceneArray::add(std::unique_ptr<Figure> fig) {
    if (!fig) return false;
    if (static_cast<int>(m_data.size()) >= SCENE_MAX_FIGURES) return false;
    m_data.push_back(std::move(fig));
    return true;
}

bool SceneArray::remove(Figure* fig) {
    if (!fig) return false;
    auto it = std::find_if(m_data.begin(), m_data.end(),
                           [&](const std::unique_ptr<Figure>& p) { return p.get() == fig; });
    if (it == m_data.end()) return false;
    m_data.erase(it);
    return true;
}

std::unique_ptr<Figure> SceneArray::extract(Figure* fig) {
    if (!fig) return nullptr;
    auto it = std::find_if(m_data.begin(), m_data.end(),
                           [&](const std::unique_ptr<Figure>& p) { return p.get() == fig; });
    if (it == m_data.end()) return nullptr;
    auto ptr = std::move(*it);
    m_data.erase(it);
    return ptr;
}

bool SceneArray::insert(std::unique_ptr<Figure> fig, int idx) {
    if (!fig) return false;
    if (static_cast<int>(m_data.size()) >= SCENE_MAX_FIGURES) return false;
    if (idx < 0) idx = 0;
    if (idx > static_cast<int>(m_data.size())) idx = static_cast<int>(m_data.size());
    m_data.insert(m_data.begin() + idx, std::move(fig));
    return true;
}

bool SceneArray::moveItem(int fromIdx, int toIdx) {
    int n = static_cast<int>(m_data.size());
    if (fromIdx < 0 || fromIdx >= n || toIdx < 0 || toIdx >= n) return false;
    if (fromIdx == toIdx) return true;

    auto temp = std::move(m_data[fromIdx]);
    m_data.erase(m_data.begin() + fromIdx);
    m_data.insert(m_data.begin() + toIdx, std::move(temp));
    return true;
}

Figure* SceneArray::get(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(m_data.size())) return nullptr;
    return m_data[idx].get();
}

void SceneArray::drawAll(sf::RenderTarget& target, float markerScale) const {
    (void)markerScale;
    for (const auto& fig : m_data) {
        fig->draw(target);
    }
}

Figure* SceneArray::hitTest(sf::Vector2f point) const {
    for (int i = static_cast<int>(m_data.size()) - 1; i >= 0; --i) {
        if (m_data[i]->contains(point)) return m_data[i].get();
    }
    return nullptr;
}

void SceneArray::clear() {
    m_data.clear();
}

} // namespace core
