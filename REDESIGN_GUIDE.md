# Руководство по переработке проекта (Графический редактор)

> **Язык:** C++17 · **Графика:** SFML 3 · **UI:** Dear ImGui + ImGui-SFML · **Сборка:** CMake · **ОС:** Linux

---

## Содержание

1. [Текущее состояние архитектуры](#1-текущее-состояние-архитектуры)
2. [Диаграмма классов — целевое состояние](#2-диаграмма-классов--целевое-состояние)
3. [Рефакторинг иерархии классов](#3-рефакторинг-иерархии-классов)
4. [Класс CompositeFigure — объединённые фигуры](#4-класс-compositefigure--объединённые-фигуры)
5. [Инструмент «Ломаная» (Polyline)](#5-инструмент-ломаная-polyline)
6. [UI — меню выбора фигур](#6-ui--меню-выбора-фигур)
7. [Режим объединения фигур](#7-режим-объединения-фигур)
8. [Исправление соединения сторон](#8-исправление-соединения-сторон)
9. [Редактирование углов](#9-редактирование-углов)
10. [Интерфейс — требования к удобству](#10-интерфейс--требования-к-удобству)
11. [Чеклист реализации](#11-чеклист-реализации)

---

## 1. Текущее состояние архитектуры

```
src/
  core/
    Figure.hpp / Figure.cpp           — базовый абстрактный класс
    CompositeFigure.hpp / .cpp        — единый класс для всех фигур:
                                          • базовые фигуры через Preset enum
                                          • составные через children[]
    Scene.hpp / Scene.cpp             — сцена
    SceneArray.hpp / SceneArray.cpp   — массив с фиксированной ёмкостью (1000)
    SceneSerializer.hpp / .cpp        — сохранение/загрузка .scene
    MathUtils.hpp                     — константы (PI, DEG_TO_RAD, rotate, …)
    Viewport.hpp                      — world↔screen
  ui/
    Toolbar.hpp / Toolbar.cpp         — панель инструментов
    PropertiesPanel.hpp / .cpp        — боковая панель свойств
    CreateFigureModal.hpp / .cpp      — контекстное меню (ПКМ)
    LayerPanel.hpp / .cpp             — панель слоёв
  utils/
    GeometryUtils.hpp / .cpp
  main.cpp                            — главный цикл (SFML + ImGui)
```

**Ключевые факты о текущей реализации:**

- `Figure` — абстрактный базовый класс. Хранит `m_vertices`, `anchor`, `parentOrigin`, `rotationAngle`, `scale`, `edges`.
- `CompositeFigure : Figure` — **один класс на всё**: базовые фигуры (через `Preset` enum: `Rectangle`, `Triangle`, `Hexagon`, `Rhombus`, `Trapezoid`, `Circle`) *и* составные (через `children[]`).
- Фабричные методы: `CompositeFigure::createRectangle(...)`, `::createTriangle(...)`, и т.д.
- `CompositeFigure` уже умеет: `setEdgeAngle` / `getEdgeAngle`, `snapChildToSiblings`, `clone`, сериализация.
- `SceneArray` — уже реализован массив с `add`, `remove`, `extract`, `hitTest`.
- `SceneSerializer` — уже реализован.

---

## 2. Диаграмма классов — целевое состояние

```
┌─────────────────────────────────────────────────────────────────────┐
│                           Figure  (abstract)                        │
│  + anchor: Vector2f                                                  │
│  + parentOrigin: Vector2f                                            │
│  + fillColor: Color                                                  │
│  + rotationAngle: float                                              │
│  + scale: Vector2f                                                   │
│  + edges: vector<Edge>                                               │
│  + lockedSides: vector<bool>                                         │
│  + lockedLengths: vector<float>                                      │
│  + parentFigure: CompositeFigure*                                    │
│  ─────────────────────────────────────────────────────────────────  │
│  + draw(target)            [virtual]                                 │
│  + contains(point): bool   [virtual]                                 │
│  + clone(): unique_ptr<>   [pure virtual]                            │
│  + typeName(): string      [pure virtual]                            │
│  + getVertices()           [virtual]                                 │
│  + getBoundingBox(): FloatRect                                       │
│  + getAbsoluteVertex(v): Vector2f                                    │
│  + move(delta)                                                       │
│  + resetAnchor()           [virtual]                                 │
│  + applyGenericSideLengths(lengths)                                  │
│  + hasSideLengths(): bool  [virtual]                                 │
│  + setSideLengths(lengths) [virtual]                                 │
│  + getSideName(idx)        [virtual]                                 │
│  + getSideLengths(): vector<float>                                   │
└───────────────────────────────────┬─────────────────────────────────┘
                                    │ inherits
              ┌─────────────────────┴──────────────────┐
              │                                         │
┌─────────────▼──────────────────────┐   ┌─────────────▼─────────────────────────┐
│         PolylineFigure              │   │          CompositeFigure               │
│  + figureName: string               │   │  + figureName: string                  │
│  ─────────────────────────────────  │   │  + isSolidGroup: bool                  │
│  + setEdgeAngle(i, angle)           │   │  + children: vector<Child>             │
│  + getEdgeAngle(i): float           │   │  struct Child {                        │
│  + setSideLengths(lengths)          │   │    unique_ptr<Figure> figure;          │
│  + getSideName(idx): const char*    │   │  }                                     │
│  + typeName(): string  ["polyline"] │   │  ─────────────────────────────────────│
│  + clone(): unique_ptr<>            │   │  + draw(target)                        │
│  # PolylineFigure()   [для подкл.] │   │  + contains(point): bool               │
└─────────────────┬───────────────────┘   │  + getVertices()                       │
                  │ inherits              │  + hitTestChild(point): Figure*        │
    ┌─────────────┼──────────────────────────────────────────┐
    │             │             │             │               │             │
┌───▼───┐  ┌─────▼────┐  ┌────▼───┐  ┌─────▼──┐  ┌────────▼──┐  ┌─────▼───┐
│  Rect │  │ Triangle │  │Hexagon │  │Rhombus │  │ Trapezoid │  │ Circle  │
│angle  │  │          │  │        │  │        │  │           │  │(60 verts│
│preset │  │ preset   │  │ preset │  │ preset │  │  preset   │  │approx.) │
└───────┘  └──────────┘  └────────┘  └────────┘  └───────────┘  └─────────┘

┌──────────────────────────────────────────────────────────────────┐
│                         SceneArray                               │
│  - m_data: array<unique_ptr<Figure>, 1000>                       │
│  - m_count: int                                                  │
│  + add(fig): bool                                                │
│  + remove(fig*): bool                                            │
│  + extract(fig*): unique_ptr<>                                   │
│  + get(idx): Figure*                                             │
│  + drawAll(target)                                               │
│  + hitTest(point): Figure*                                       │
│  + clear()                                                       │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                           Scene                                  │
│  - m_figures: SceneArray                                         │
│  - m_selectedFigure: Figure*                                     │
│  + addFigure(fig)                                                │
│  + removeFigure(fig*)                                            │
│  + extractFigure(fig*): unique_ptr<>                             │
│  + hitTest(point): Figure*                                       │
│  + drawAll(target, markerScale)                                  │
│  + figureCount(): int                                            │
│  + getFigure(idx): Figure*                                       │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                      UserFigureRegistry                          │
│  struct UserFigureType {                                         │
│    string name;                                                  │
│    function<unique_ptr<Figure>()> factory;  // клон-шаблон       │
│  }                                                               │
│  + vector<UserFigureType> types;                                 │
│  + registerFigure(name, templateFig)                             │
│  + create(index): unique_ptr<Figure>                             │
└──────────────────────────────────────────────────────────────────┘
```

---

## 3. Рефакторинг иерархии классов

### 3.1 Создать класс `PolylineFigure`

**Файл:** `src/core/PolylineFigure.hpp` *(новый)*

```cpp
#pragma once
#include "Figure.hpp"
#include <string>

namespace core {

/// Фигура, задаваемая произвольным набором вершин (ломаная, автоматически замыкается).
/// Базовые фигуры (Rectangle, Triangle, …) станут подклассами PolylineFigure.
class PolylineFigure : public Figure {
public:
    std::string figureName;

    explicit PolylineFigure(std::vector<sf::Vector2f> vertices, std::string name = "Custom");

    bool hasSideLengths()  const override { return true; }
    void setSideLengths(const std::vector<float>& lengths) override;
    const char* getSideName(int idx) const override;

    std::string typeName() const override { return "polyline"; }
    std::unique_ptr<Figure> clone() const override;

    /// Задать угол i-го ребра (в градусах) и скорректировать вершину i+1
    void setEdgeAngle(int edgeIdx, float angleDeg);
    float getEdgeAngle(int edgeIdx) const;

protected:
    PolylineFigure() = default; // для подклассов (базовых фигур)
};

} // namespace core
```

**Файл:** `src/core/PolylineFigure.cpp` *(новый)*

```cpp
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
    for (int j = next + 1; j < n; ++j)
        m_vertices[j] += delta;
}

float PolylineFigure::getEdgeAngle(int edgeIdx) const {
    int n = static_cast<int>(m_vertices.size());
    if (n < 2 || edgeIdx < 0 || edgeIdx >= n) return 0.f;
    int next = (edgeIdx + 1) % n;
    return std::atan2(m_vertices[next].y - m_vertices[edgeIdx].y,
                      m_vertices[next].x - m_vertices[edgeIdx].x)
           * 180.f / math::PI;
}

} // namespace core
```

### 3.2 Базовые фигуры как подклассы `PolylineFigure`

Создать файлы `src/core/Figures.hpp` и `src/core/Figures.cpp`.

Каждая базовая фигура наследуется от `PolylineFigure`, задаёт вершины в конструкторе, переопределяет `typeName()` и `getSideName()`.

**Пример для Rectangle:**

```cpp
// Figures.hpp
class Rectangle : public PolylineFigure {
public:
    Rectangle(float width, float height);
    std::string typeName() const override { return "rectangle"; }
    const char* getSideName(int idx) const override;
    void setSideLengths(const std::vector<float>& lengths) override;
    std::unique_ptr<Figure> clone() const override;
private:
    float m_width, m_height;
};

// Figures.cpp
Rectangle::Rectangle(float width, float height)
    : m_width(width), m_height(height) {
    figureName = "Rectangle";
    m_vertices = {
        {-width/2.f, -height/2.f},
        { width/2.f, -height/2.f},
        { width/2.f,  height/2.f},
        {-width/2.f,  height/2.f}
    };
    edges.resize(4);
}
```

Аналогично для `Triangle`, `Hexagon`, `Rhombus`, `Trapezoid`, `Circle`.

> **Важно:** убрать `enum class Preset` из `CompositeFigure`. Базовые фигуры теперь отдельные классы. `CompositeFigure` используется **только** для составных фигур (из нескольких детей).

### 3.3 Обновить `CompositeFigure`

После вынесения базовых фигур:

- Убрать `Preset` enum и все `m_cached*` поля.
- Убрать фабричные методы `createRectangle`, `createTriangle` и т.д.
- Убрать `setSideLengths` / `getSideName` / `hasSideLengths` — они не нужны для составных фигур.
- Оставить: `children`, `figureName`, `isSolidGroup`, `draw`, `contains`, `getVertices`, `hitTestChild`, `snapChildToSiblings`, `clone`, `insertChild`, `extractChild`, `moveChild`.
- Добавить: `setEdgeAngle` / `getEdgeAngle` перенести в `PolylineFigure`.

---

## 4. Класс CompositeFigure — объединённые фигуры

### 4.1 Структура

```cpp
// CompositeFigure.hpp
class CompositeFigure : public Figure {
public:
    std::string figureName;
    bool isSolidGroup = false;

    struct Child {
        std::unique_ptr<Figure> figure;
    };
    std::vector<Child> children;

    // Создаёт CompositeFigure из списка Figure* на сцене
    static std::unique_ptr<CompositeFigure> mergeFromScene(
        const std::vector<Figure*>& figs,
        Scene& scene,
        const std::string& name
    );

    // Извлечь дочерний элемент обратно на сцену
    std::unique_ptr<Figure> extractChild(Figure* childPtr);

    // Вставить фигуру как ребёнка
    void insertChild(std::unique_ptr<Figure> fig, int index, sf::Vector2f localOffset, float localRotation = 0.f);

    bool moveChild(int fromIdx, int toIdx);
    Figure* hitTestChild(sf::Vector2f point) const;
    bool snapChildToSiblings(Figure* child);

    void draw(sf::RenderTarget& target) const override;
    bool contains(sf::Vector2f point) const override;
    const std::vector<sf::Vector2f>& getVertices() const override;

    std::string typeName() const override { return "composite"; }
    std::unique_ptr<Figure> clone() const override;
    void applyScale() override;
    void resetAnchor() override;
    void setAnchorKeepAbsolute(sf::Vector2f newAnchor) override;

private:
    mutable std::vector<sf::Vector2f> m_cachedVerts;
};
```

### 4.2 Метод `mergeFromScene`

```cpp
std::unique_ptr<CompositeFigure> CompositeFigure::mergeFromScene(
    const std::vector<Figure*>& figs, Scene& scene, const std::string& name)
{
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
        ptr->parentFigure = compound.get();
        ptr->anchor       = relOffset;
        ptr->parentOrigin = {0.f, 0.f};
        c.figure = std::move(ptr);
        compound->children.push_back(std::move(c));
    }
    return compound;
}
```

---

## 5. Инструмент «Ломаная» (Polyline)

### 5.1 Новый инструмент

Добавить в `ui::Tool` (Toolbar.hpp):

```cpp
enum class Tool {
    Select,
    // Базовые фигуры:
    Rectangle, Triangle, Hexagon, Rhombus, Trapezoid, Circle,
    // Новые:
    Polyline,        // рисование ломаной кликами мыши
    CompoundSelect,  // выбор фигур для объединения
    // Пользовательские фигуры хранятся отдельно (CustomFigure + index)
};
```

### 5.2 Состояние в `main.cpp`

```cpp
bool isDrawingPolyline = false;
std::vector<sf::Vector2f> polylinePoints;
// Предпросмотр: позиция мыши
sf::Vector2f polylinePreviewPoint;
```

### 5.3 Логика рисования

```cpp
// При каждом ЛКМ в режиме Polyline:
if (tool == Tool::Polyline && event.type == sf::Event::MouseButtonPressed
    && event.mouseButton.button == sf::Mouse::Left)
{
    sf::Vector2f worldPos = viewport.screenToWorld(mousePos);
    
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !polylinePoints.empty()) {
        worldPos = snapToAngle(polylinePoints.back(), worldPos); // привязка к 15°
    }
    
    if (!isDrawingPolyline) {
        isDrawingPolyline = true;
        polylinePoints.clear();
    }
    polylinePoints.push_back(worldPos);
}

// Двойной клик или Enter → завершить и замкнуть
if (isDrawingPolyline && polylinePoints.size() >= 3) {
    if (doubleClick || enterPressed) {
        finishPolyline();
    }
}

// Escape → отменить
if (isDrawingPolyline && escapePressed) {
    isDrawingPolyline = false;
    polylinePoints.clear();
}
```

### 5.4 Привязка к углу (Shift)

```cpp
sf::Vector2f snapToAngle(sf::Vector2f from, sf::Vector2f to) {
    float dx = to.x - from.x, dy = to.y - from.y;
    float len   = std::hypot(dx, dy);
    float angle = std::atan2(dy, dx);
    // Шаг 15 градусов = PI/12
    float snapped = std::round(angle / (math::PI / 12.f)) * (math::PI / 12.f);
    return { from.x + len * std::cos(snapped), from.y + len * std::sin(snapped) };
}
```

### 5.5 Завершение ломаной

```cpp
void finishPolyline() {
    // Центрировать вершины
    sf::Vector2f center = {0.f, 0.f};
    for (auto& p : polylinePoints) center += p;
    center /= static_cast<float>(polylinePoints.size());

    std::vector<sf::Vector2f> localVerts;
    for (auto& p : polylinePoints) localVerts.push_back(p - center);

    // ВАЖНО: ломаная автоматически замкнута (последнее ребро от хвоста к голове)
    // PolylineFigure::draw() уже рисует замкнутый контур (как ConvexShape)

    // Запросить имя через ImGui InputText диалог (showPolylineNameDialog = true)
    pendingPolylineVerts = localVerts;
    pendingPolylineCenter = center;
    showPolylineNameDialog = true;

    isDrawingPolyline = false;
    polylinePoints.clear();
}

// После подтверждения имени:
auto fig = std::make_unique<core::PolylineFigure>(pendingPolylineVerts, enteredName);
fig->anchor = pendingPolylineCenter;
scene.addFigure(std::move(fig));
// Зарегистрировать в UserFigureRegistry (см. раздел 6)
```

### 5.6 Предпросмотр при рисовании

В главном цикле после `scene.drawAll`:

```cpp
if (isDrawingPolyline && polylinePoints.size() >= 1) {
    // Нарисовать уже добавленные вершины
    sf::VertexArray lines(sf::PrimitiveType::LineStrip, polylinePoints.size() + 1);
    for (size_t i = 0; i < polylinePoints.size(); ++i) {
        lines[i].position = viewport.worldToScreen(polylinePoints[i]);
        lines[i].color    = sf::Color::Cyan;
    }
    // Предпросмотр до текущей позиции мыши
    lines[polylinePoints.size()].position = currentMouseScreen;
    lines[polylinePoints.size()].color    = sf::Color(0, 200, 200, 128);
    window.draw(lines);

    // Отобразить точки
    for (auto& p : polylinePoints) {
        sf::CircleShape dot(4.f);
        dot.setFillColor(sf::Color::Yellow);
        dot.setOrigin(4.f, 4.f);
        dot.setPosition(viewport.worldToScreen(p));
        window.draw(dot);
    }
}
```

---

## 6. UI — меню выбора фигур

### 6.1 UserFigureRegistry

```cpp
// main.cpp или отдельный файл src/core/UserFigureRegistry.hpp
struct UserFigureType {
    std::string name;
    std::function<std::unique_ptr<core::Figure>()> factory;
};
std::vector<UserFigureType> userFigureTypes;

void registerUserFigure(const std::string& name, const core::Figure* templateFig) {
    auto snapshot = templateFig->clone();
    userFigureTypes.push_back({
        name,
        [snap = std::shared_ptr<core::Figure>(std::move(snapshot))]() {
            return snap->clone();
        }
    });
}
```

### 6.2 Отображение в Toolbar

В `Toolbar::render()` добавить выпадающий список пользовательских фигур:

```cpp
// Стандартные фигуры — основные кнопки
if (ImGui::Button("Rectangle")) { ... }
// ...

ImGui::Separator();
ImGui::Text("Custom:");
for (int i = 0; i < (int)userFigureTypes.size(); ++i) {
    bool selected = (tool == Tool::CustomFigure && customFigureIndex == i);
    if (ImGui::Selectable(userFigureTypes[i].name.c_str(), selected)) {
        tool = Tool::CustomFigure;
        customFigureIndex = i;
    }
}
```

Добавить `Tool::CustomFigure` и переменную `int customFigureIndex` в состояние главного цикла.

### 6.3 Размещение пользовательской фигуры на сцене

При клике в режиме `Tool::CustomFigure`:

```cpp
auto fig = userFigureTypes[customFigureIndex].factory();
fig->anchor = viewport.screenToWorld(mousePos);
scene.addFigure(std::move(fig));
```

### 6.4 CreateFigureModal (ПКМ)

В `CreateFigureModal` добавить секцию «Custom Figures» — список пользовательских фигур с возможностью выбора так же, как стандартные.

---

## 7. Режим объединения фигур

### 7.1 Режим `CompoundSelect`

**Состояние в `main.cpp`:**

```cpp
std::vector<core::Figure*> compoundSelection;
bool showMergeDialog    = false;
char mergeNameBuf[256]  = "Compound";
```

**При клике в режиме `CompoundSelect`:**

```cpp
core::Figure* hit = scene.hitTest(worldPos);
if (hit) {
    auto it = std::find(compoundSelection.begin(), compoundSelection.end(), hit);
    if (it == compoundSelection.end()) {
        compoundSelection.push_back(hit);   // выделить
    } else {
        compoundSelection.erase(it);         // повторный клик = снять выделение
    }
}
```

**Визуальное выделение:** В `scene.drawAll` (или отдельным проходом) рисовать цветной контур вокруг фигур из `compoundSelection`.

### 7.2 Кнопка «Merge»

В ImGui-панели (боковая панель или отдельный попап):

```cpp
if (tool == Tool::CompoundSelect) {
    ImGui::Text("Selected: %d figures", (int)compoundSelection.size());
    if (compoundSelection.size() >= 2) {
        if (ImGui::Button("Merge Selected")) {
            showMergeDialog = true;
        }
    }
    if (ImGui::Button("Cancel Selection")) {
        compoundSelection.clear();
    }
}

if (showMergeDialog) {
    ImGui::OpenPopup("Merge Figures");
}
if (ImGui::BeginPopupModal("Merge Figures", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::InputText("Name", mergeNameBuf, sizeof(mergeNameBuf));
    if (ImGui::Button("OK") && strlen(mergeNameBuf) > 0) {
        auto compound = core::CompositeFigure::mergeFromScene(
            compoundSelection, scene, mergeNameBuf);
        if (compound) {
            registerUserFigure(mergeNameBuf, compound.get()); // регистрируем шаблон
            scene.addFigure(std::move(compound));
        }
        compoundSelection.clear();
        showMergeDialog = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        showMergeDialog = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
```

### 7.3 Удаление дочерней фигуры из составной

В `PropertiesPanel::render()`, если выделена `CompositeFigure`:

```cpp
if (auto* cf = dynamic_cast<core::CompositeFigure*>(selectedFig)) {
    ImGui::Separator();
    ImGui::Text("Children (%d):", (int)cf->children.size());
    for (int i = 0; i < (int)cf->children.size(); ++i) {
        auto* child = cf->children[i].figure.get();
        ImGui::Text("[%d] %s", i, child->typeName().c_str());
        ImGui::SameLine();
        std::string btnId = "Extract##child" + std::to_string(i);
        if (ImGui::Button(btnId.c_str())) {
            auto extracted = cf->extractChild(child);
            // Восстановить абсолютную позицию
            extracted->parentFigure = nullptr;
            extracted->parentOrigin = {0.f, 0.f};
            // anchor уже хранит абсолютную позицию, т.к. extractChild её восстанавливает
            scene.addFigure(std::move(extracted));
            break; // итератор сломан
        }
        ImGui::SameLine();
        std::string delId = "Delete##child" + std::to_string(i);
        if (ImGui::Button(delId.c_str())) {
            cf->children.erase(cf->children.begin() + i);
            break;
        }
    }
}
```

> **Важно:** `extractChild` должен правильно восстанавливать абсолютный `anchor` дочерней фигуры перед возвратом. Добавить это в реализацию:
> ```cpp
> std::unique_ptr<Figure> CompositeFigure::extractChild(Figure* childPtr) {
>     for (auto it = children.begin(); it != children.end(); ++it) {
>         if (it->figure.get() == childPtr) {
>             auto ptr = std::move(it->figure);
>             children.erase(it);
>             // Восстановить абсолютное положение
>             sf::Vector2f absAnchor = getAbsoluteAnchor() + ptr->anchor;
>             ptr->parentFigure = nullptr;
>             ptr->anchor       = absAnchor;
>             ptr->parentOrigin = {0.f, 0.f};
>             return ptr;
>         }
>     }
>     return nullptr;
> }
> ```

---

## 8. Исправление соединения сторон

### Проблема

В текущей реализации `Figure::draw` рисует **каждое ребро отдельным `sf::ConvexShape` (квадом из 4 точек)**. Из-за этого концы соседних рёбер не совпадают — между ними видны зазоры, особенно при толстых линиях.

### Решение

Заменить рисование рёбер на **miter-join** (острое соединение) или использовать один непрерывный контур.

**Вариант 1 (рекомендуемый) — `sf::VertexArray` с `sf::PrimitiveType::TriangleStrip`:**

```cpp
// В Figure::draw(), вместо отдельных квадов:
void Figure::draw(sf::RenderTarget& target) const {
    const auto& verts = getVertices();
    if (verts.empty()) return;
    size_t n = verts.size();

    // 1. Заливка
    sf::ConvexShape fill(n);
    for (size_t i = 0; i < n; ++i)
        fill.setPoint(i, verts[i]);
    fill.setPosition(getAbsoluteAnchor());
    fill.setRotation(getAbsoluteRotation());
    fill.setScale(getAbsoluteScale());
    fill.setFillColor(fillColor);
    fill.setOutlineThickness(0.f);
    target.draw(fill);

    // 2. Контур — один непрерывный VertexArray с miter-join
    if (edges.empty()) return;

    // Вычислить абсолютные позиции вершин
    std::vector<sf::Vector2f> absV(n);
    for (size_t i = 0; i < n; ++i)
        absV[i] = getAbsoluteVertex(verts[i]);

    drawThickPolyline(target, absV, edges, /*closed=*/true);
}
```

Реализовать утилитарную функцию `drawThickPolyline` в `GeometryUtils`:

```cpp
// GeometryUtils.hpp
void drawThickPolyline(
    sf::RenderTarget& target,
    const std::vector<sf::Vector2f>& pts,
    const std::vector<core::Edge>& edges,
    bool closed);

// GeometryUtils.cpp
// Для каждого ребра вычислять miter-вектор в узловых точках
// (среднее двух нормалей соседних рёбер, ограниченное miter_limit).
// Строить sf::VertexArray TriangleStrip: пары точек по обе стороны ребра.
// Для per-edge цветов — отдельный TriangleStrip для каждого ребра (или единый, если все одного цвета).
```

**Алгоритм miter-join для каждой вершины `i`:**

```
n0 = нормаль ребра (i-1, i)  // нормализованная перпендикулярная
n1 = нормаль ребра (i, i+1)
miter = normalize(n0 + n1)
miter_length = half_width / dot(miter, n0)
miter_length = min(miter_length, miter_limit * half_width)
corner_pos± = pts[i] ± miter * miter_length
```

Если нужна поддержка разных `edges[i].color` — рисовать каждое ребро отдельным `sf::VertexArray` с `sf::TriangleStrip` (4 вершины), но с правильными miter-точками на концах.

---

## 9. Редактирование углов

### 9.1 В `PropertiesPanel`

Для `PolylineFigure` (и её подклассов) показывать таблицу рёбер:

```cpp
if (auto* pl = dynamic_cast<core::PolylineFigure*>(selectedFig)) {
    ImGui::Separator();
    ImGui::Text("Edge Angles:");
    int n = (int)pl->getVertices().size();
    for (int i = 0; i < n; ++i) {
        float angle = pl->getEdgeAngle(i);
        std::string label = "##ea" + std::to_string(i);
        ImGui::Text("Edge %d:", i);
        ImGui::SameLine();
        if (ImGui::InputFloat(label.c_str(), &angle, 1.f, 5.f, "%.1f°")) {
            pl->setEdgeAngle(i, angle);
        }
    }
}
```

### 9.2 При рисовании ломаной

Зажатая `Shift` → привязка угла нового ребра к кратному **15°** (уже описано в разделе 5.4).

### 9.3 Редактирование через drag вершин

Дополнительно (опционально, но улучшает UX): в режиме `Select`, если выбрана `PolylineFigure`, рисовать маркеры (кружки) на каждой вершине. Drag маркера = редактирование вершины.

```cpp
// При отрисовке, если selectedFig — PolylineFigure:
if (auto* pl = dynamic_cast<core::PolylineFigure*>(selectedFig)) {
    for (int i = 0; i < (int)pl->getVertices().size(); ++i) {
        sf::Vector2f absV = pl->getAbsoluteVertex(pl->getVertices()[i]);
        sf::Vector2f screenV = viewport.worldToScreen(absV);
        sf::CircleShape marker(5.f);
        marker.setFillColor(sf::Color::Yellow);
        marker.setOrigin(5.f, 5.f);
        marker.setPosition(screenV);
        window.draw(marker);
    }
}
// Drag маркера → изменить pl->getVerticesMutable()[i]
```

---

## 10. Интерфейс — требования к удобству

### Горячие клавиши

| Клавиша           | Действие                                     |
|-------------------|----------------------------------------------|
| `P`               | Инструмент Polyline                          |
| `M`               | Режим CompoundSelect                         |
| `Enter`           | Завершить ломаную / подтвердить объединение  |
| `Escape`          | Отменить текущую операцию                    |
| `Delete`          | Удалить выбранную фигуру                     |
| `Shift` (при рисовании) | Привязка угла к 15°                  |
| `Ctrl+S`          | Сохранить сцену                              |
| `Ctrl+O`          | Загрузить сцену                              |
| `Ctrl+Z`          | Отменить (если реализовано)                  |

### Визуальная обратная связь

- **CompoundSelect**: выделенные фигуры обводятся пунктирным прямоугольником (highlight).
- **Polyline**: предпросмотр линии от последней точки до курсора, точки отмечены кружками.
- **Drag вершин**: курсор меняется при наведении на маркер (или маркер увеличивается).
- **Привязка угла**: при Shift показывать маленькую стрелку на угол привязки.

### Панель свойств

- Для `CompositeFigure` — список детей с именами, кнопки «Extract» (вернуть на сцену) и «Delete» (удалить).
- Для `PolylineFigure` — таблица рёбер: длина и угол каждого ребра.
- Блокировка сторон (уже реализована через `lockedSides`) — оставить как есть.

### Toolbar

- Базовые фигуры: Rectangle, Triangle, Hexagon, Rhombus, Trapezoid, Circle.
- Polyline (кнопка или иконка).
- CompoundSelect (кнопка или иконка).
- Разделитель + секция «Custom Figures» (пользовательские фигуры).
- Кнопки Save / Load.

---

## 11. Чеклист реализации

### Шаг 1 — Иерархия классов

- [ ] Создать `src/core/PolylineFigure.hpp` и `PolylineFigure.cpp`
- [ ] Создать `src/core/Figures.hpp` и `Figures.cpp` (Rectangle, Triangle, Hexagon, Rhombus, Trapezoid, Circle как подклассы `PolylineFigure`)
- [ ] Убрать `Preset` enum из `CompositeFigure`, убрать фабричные методы (`createRectangle` и т.д.)
- [ ] Обновить `CompositeFigure` — оставить только логику составных фигур
- [ ] Перенести `setEdgeAngle` / `getEdgeAngle` в `PolylineFigure`
- [ ] Обновить `Scene::addFigure` и `CreateFigureModal` — теперь создавать `Rectangle(...)` вместо `CompositeFigure::createRectangle(...)`
- [ ] Обновить `CMakeLists.txt` — добавить новые `.cpp` файлы

### Шаг 2 — Инструмент Polyline

- [ ] Добавить `Tool::Polyline` и `Tool::CompoundSelect` (и `Tool::CustomFigure`) в `Toolbar.hpp`
- [ ] Добавить кнопки в `Toolbar.cpp`
- [ ] Реализовать логику рисования ломаной (клики, Shift-привязка, предпросмотр) в `main.cpp`
- [ ] Диалог имени для новой ломаной (ImGui Popup с InputText)
- [ ] Ломаная автоматически замыкается (PolylineFigure рисует замкнутый контур)
- [ ] Регистрировать ломаную в `userFigureTypes` после создания

### Шаг 3 — UserFigureRegistry и меню

- [ ] Добавить `UserFigureRegistry` (структура + вектор) в `main.cpp` или отдельный файл
- [ ] Отображать пользовательские фигуры в `Toolbar`
- [ ] Отображать пользовательские фигуры в `CreateFigureModal`
- [ ] При клике на пользовательскую фигуру — создавать через `factory()`

### Шаг 4 — Режим объединения

- [ ] Реализовать режим `CompoundSelect` (выделение кликами, визуальный highlight)
- [ ] Кнопка «Merge» + диалог имени
- [ ] `CompositeFigure::mergeFromScene` — реализовать
- [ ] После merge — зарегистрировать в `userFigureTypes`
- [ ] Кнопки «Extract» и «Delete» для детей составной фигуры в `PropertiesPanel`
- [ ] `extractChild` — корректно восстанавливать абсолютный anchor

### Шаг 5 — Исправление соединения сторон

- [ ] Реализовать `drawThickPolyline` с miter-join в `GeometryUtils`
- [ ] Обновить `Figure::draw` — использовать `drawThickPolyline` вместо отдельных квадов
- [ ] Проверить корректность для замкнутых форм и открытых ломаных

### Шаг 6 — Редактирование углов

- [ ] В `PropertiesPanel`: таблица рёбер с полем угла для `PolylineFigure`
- [ ] (Опционально) Drag-маркеры вершин в режиме Select

### Шаг 7 — Верификация

- [ ] Собрать проект: `cd build && cmake .. && make -j$(nproc)`
- [ ] Нарисовать ломаную из 5 точек → убедиться что замыкается
- [ ] Нарисовать ломаную с Shift → углы кратны 15°
- [ ] Создать `CompositeFigure` из двух фигур → имя появляется в Custom-меню
- [ ] Разместить пользовательскую фигуру через меню
- [ ] Удалить/извлечь дочернюю фигуру из составной
- [ ] Проверить стыковку рёбер (нет зазоров при толстых линиях)
- [ ] Сохранить и загрузить сцену

---

*Последнее обновление: 2026-03-27*
