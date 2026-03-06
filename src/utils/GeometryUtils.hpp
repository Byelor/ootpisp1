#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

namespace core::geometry {

    /**
     * @brief Вычисляет пересечение двух линий, заданных точкой и направлением
     * @param p1 Точка на первой линии
     * @param d1 Направление первой линии
     * @param p2 Точка на второй линии
     * @param d2 Направление второй линии
     * @param intersection [out] Точка пересечения (если есть)
     * @return true если линии пересекаются (не параллельны)
     */
    bool lineIntersection(sf::Vector2f p1, sf::Vector2f d1,
        sf::Vector2f p2, sf::Vector2f d2,
        sf::Vector2f& intersection);

    /**
     * @brief Проверяет, находится ли точка внутри полигона (алгоритм ray casting)
     * @param point Точка для проверки
     * @param polygon Вектор вершин полигона (в порядке обхода)
     * @return true если точка внутри полигона
     */
    bool pointInPolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Релаксация вершин для достижения заданных длин сторон
     * @param vertices Вектор вершин (будет изменен)
     * @param targetLengths Целевые длины сторон
     * @param iterations Количество итераций
     * @param stiffness Жесткость пружин (0.0 - 1.0)
     */
    void relaxEdges(std::vector<sf::Vector2f>& vertices,
        const std::vector<float>& targetLengths,
        int iterations = 500,
        float stiffness = 0.5f);

    /**
     * @brief Вычисляет ограничивающий прямоугольник для набора точек
     * @param points Вектор точек
     * @return sf::FloatRect с координатами (left, top, width, height)
     */
    sf::FloatRect computeBoundingBox(const std::vector<sf::Vector2f>& points);

    /**
     * @brief Триангуляция полигона (метод "ухорезки")
     * @param polygon Вектор вершин полигона (по часовой стрелке)
     * @return Вектор треугольников (каждый треугольник - 3 индекса)
     */
    std::vector<size_t> triangulatePolygon(const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Проверяет, является ли полигон выпуклым
     * @param polygon Вектор вершин
     * @return true если полигон выпуклый
     */
    bool isConvexPolygon(const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Вычисляет площадь полигона
     * @param polygon Вектор вершин
     * @return Площадь (положительная для обхода по часовой стрелке)
     */
    float polygonArea(const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Вычисляет центр масс полигона
     * @param polygon Вектор вершин
     * @return Центр масс
     */
    sf::Vector2f polygonCentroid(const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Масштабирует полигон относительно центра
     * @param polygon Вектор вершин (будет изменен)
     * @param scale Коэффициент масштабирования
     */
    void scalePolygon(std::vector<sf::Vector2f>& polygon, float scale);

    /**
     * @brief Поворачивает полигон вокруг заданной точки
     * @param polygon Вектор вершин (будет изменен)
     * @param center Центр поворота
     * @param angle Угол в радианах
     */
    void rotatePolygon(std::vector<sf::Vector2f>& polygon,
        sf::Vector2f center,
        float angle);

    /**
     * @brief Находит ближайшую точку на отрезке к заданной точке
     * @param p Точка
     * @param a Начало отрезка
     * @param b Конец отрезка
     * @return Ближайшая точка на отрезке
     */
    sf::Vector2f closestPointOnSegment(sf::Vector2f p,
        sf::Vector2f a,
        sf::Vector2f b);

    /**
     * @brief Вычисляет расстояние от точки до отрезка
     * @param p Точка
     * @param a Начало отрезка
     * @param b Конец отрезка
     * @return Расстояние
     */
    float distanceToSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b);

    /**
     * @brief Проверяет, пересекаются ли два отрезка
     * @param a1 Начало первого отрезка
     * @param a2 Конец первого отрезка
     * @param b1 Начало второго отрезка
     * @param b2 Конец второго отрезка
     * @param intersection [out] Точка пересечения (опционально)
     * @return true если отрезки пересекаются
     */
    bool segmentsIntersect(sf::Vector2f a1, sf::Vector2f a2,
        sf::Vector2f b1, sf::Vector2f b2,
        sf::Vector2f* intersection = nullptr);

    /**
     * @brief Структура для хранения информации о пересечении луча с полигоном
     */
    struct RaycastHit {
        bool hit = false;           // Было ли пересечение
        float distance = 0.f;       // Расстояние до точки пересечения
        sf::Vector2f point;         // Точка пересечения
        size_t edgeIndex = 0;       // Индекс пересеченного ребра
    };

    /**
     * @brief Бросает луч из точки в заданном направлении и находит пересечение с полигоном
     * @param origin Начало луча
     * @param direction Направление луча (нормализованное)
     * @param polygon Вектор вершин полигона
     * @return Информация о пересечении
     */
    RaycastHit raycastPolygon(sf::Vector2f origin,
        sf::Vector2f direction,
        const std::vector<sf::Vector2f>& polygon);

    /**
     * @brief Упрощает полигон, удаляя коллинеарные точки
     * @param polygon Вектор вершин
     * @param epsilon Допуск коллинеарности
     * @return Упрощенный полигон
     */
    std::vector<sf::Vector2f> simplifyPolygon(const std::vector<sf::Vector2f>& polygon,
        float epsilon = 0.1f);

    /**
     * @brief Создает полигон - оболочку вокруг набора точек (алгоритм Грэхема)
     * @param points Набор точек
     * @return Выпуклая оболочка
     */
    std::vector<sf::Vector2f> convexHull(const std::vector<sf::Vector2f>& points);

} // namespace core::geometry