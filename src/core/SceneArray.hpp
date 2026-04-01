#pragma once
#include "Figure.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace core {

constexpr int SCENE_MAX_FIGURES = 1000;

/// Массив фигур с ограничением на максимальное количество.
/// Хранит raw-указатели (владение остаётся за unique_ptr в SceneArray).
class SceneArray {
public:
    SceneArray() { m_data.reserve(SCENE_MAX_FIGURES); }

    /// Добавить фигуру в конец. Возвращает false если массив полон.
    bool add(std::unique_ptr<Figure> fig);

    /// Удалить фигуру по указателю. Сдвигает оставшиеся элементы влево.
    /// Возвращает false если не найдена.
    bool remove(Figure* fig);

    /// Извлечь фигуру (передать владение вызывающему).
    std::unique_ptr<Figure> extract(Figure* fig);

    /// Вставить фигуру по индексу.
    bool insert(std::unique_ptr<Figure> fig, int idx);

    /// Сдвинуть фигуру с одного индекса на другой (для drag and drop).
    bool moveItem(int fromIdx, int toIdx);

    /// Количество фигур
    int count() const { return static_cast<int>(m_data.size()); }

    /// Доступ по индексу
    Figure* get(int idx) const;

    /// Вызвать draw для каждой фигуры (процедура отображения)
    void drawAll(sf::RenderTarget& target, float markerScale) const;

    /// Итерация (для hitTest и др.)
    Figure* hitTest(sf::Vector2f point) const;

    /// Очистить массив
    void clear();

private:
    std::vector<std::unique_ptr<Figure>> m_data;
};

} // namespace core
