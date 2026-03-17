#pragma once
#include "Figure.hpp"
#include <array>
#include <cstddef>
#include <memory>

namespace core {

constexpr int SCENE_MAX_FIGURES = 1000;

/// Массив фигур с фиксированной ёмкостью.
/// Хранит raw-указатели (владение остаётся за unique_ptr в SceneArray).
class SceneArray {
public:
    SceneArray() = default;

    /// Добавить фигуру в конец. Возвращает false если массив полон.
    bool add(std::unique_ptr<Figure> fig);

    /// Удалить фигуру по указателю. Сдвигает оставшиеся элементы влево.
    /// Возвращает false если не найдена.
    bool remove(Figure* fig);

    /// Извлечь фигуру (передать владение вызывающему).
    std::unique_ptr<Figure> extract(Figure* fig);

    /// Количество фигур
    int count() const { return m_count; }

    /// Доступ по индексу
    Figure* get(int idx) const;

    /// Вызвать draw для каждой фигуры (процедура отображения)
    void drawAll(sf::RenderTarget& target, float markerScale) const;

    /// Итерация (для hitTest и др.)
    Figure* hitTest(sf::Vector2f point) const;

    /// Очистить массив
    void clear();

private:
    std::array<std::unique_ptr<Figure>, SCENE_MAX_FIGURES> m_data;
    int m_count = 0;

    /// Сдвинуть элементы влево начиная с позиции pos
    void shiftLeft(int pos);
};

} // namespace core
