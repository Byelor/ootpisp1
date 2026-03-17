#include "SceneArray.hpp"

namespace core {

bool SceneArray::add(std::unique_ptr<Figure> fig) {
    if (!fig || m_count >= SCENE_MAX_FIGURES) return false;
    m_data[m_count++] = std::move(fig);
    return true;
}

void SceneArray::shiftLeft(int pos) {
    for (int i = pos; i < m_count - 1; ++i)
        m_data[i] = std::move(m_data[i + 1]);
    m_data[m_count - 1].reset();
    --m_count;
}

bool SceneArray::remove(Figure* fig) {
    for (int i = 0; i < m_count; ++i) {
        if (m_data[i].get() == fig) {
            shiftLeft(i);
            return true;
        }
    }
    return false;
}

std::unique_ptr<Figure> SceneArray::extract(Figure* fig) {
    for (int i = 0; i < m_count; ++i) {
        if (m_data[i].get() == fig) {
            auto ptr = std::move(m_data[i]);
            shiftLeft(i);
            return ptr;
        }
    }
    return nullptr;
}

Figure* SceneArray::get(int idx) const {
    if (idx < 0 || idx >= m_count) return nullptr;
    return m_data[idx].get();
}

void SceneArray::drawAll(sf::RenderTarget& target, float markerScale) const {
    for (int i = 0; i < m_count; ++i) {
        m_data[i]->draw(target);
    }
}

Figure* SceneArray::hitTest(sf::Vector2f point) const {
    for (int i = m_count - 1; i >= 0; --i) {
        if (m_data[i]->contains(point))
            return m_data[i].get();
    }
    return nullptr;
}

void SceneArray::clear() {
    for (int i = 0; i < m_count; ++i) {
        m_data[i].reset();
    }
    m_count = 0;
}

} // namespace core
