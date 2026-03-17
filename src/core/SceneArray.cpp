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

bool SceneArray::insert(std::unique_ptr<Figure> fig, int idx) {
    if (!fig || m_count >= SCENE_MAX_FIGURES) return false;
    if (idx < 0) idx = 0;
    if (idx > m_count) idx = m_count;

    for (int i = m_count; i > idx; --i) {
        m_data[i] = std::move(m_data[i - 1]);
    }
    m_data[idx] = std::move(fig);
    ++m_count;
    return true;
}

bool SceneArray::moveItem(int fromIdx, int toIdx) {
    if (fromIdx < 0 || fromIdx >= m_count || toIdx < 0 || toIdx >= m_count) return false;
    if (fromIdx == toIdx) return true;

    auto temp = std::move(m_data[fromIdx]);
    if (fromIdx < toIdx) {
        for (int i = fromIdx; i < toIdx; ++i) {
            m_data[i] = std::move(m_data[i + 1]);
        }
    } else {
        for (int i = fromIdx; i > toIdx; --i) {
            m_data[i] = std::move(m_data[i - 1]);
        }
    }
    m_data[toIdx] = std::move(temp);
    return true;
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
