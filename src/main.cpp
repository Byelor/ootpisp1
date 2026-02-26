#include "core/Figures.hpp"
#include "core/Scene.hpp"
#include "ui/PropertiesPanel.hpp"
#include "ui/Toolbar.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <memory>

int main() {
  sf::RenderWindow window(sf::VideoMode(1280, 720), "GraphEditor");
  window.setFramerateLimit(60);

  if (!ImGui::SFML::Init(window)) {
    std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    return -1;
  }

  sf::Clock deltaClock;

  core::Scene scene;

  ui::Toolbar toolbar;
  ui::PropertiesPanel propertiesPanel;
  ui::Tool currentTool = ui::Tool::Select;

  bool isDragging = false;
  bool isCreating = false;
  sf::Vector2f dragOffset;
  sf::Vector2f createStartPos;

  bool showGrid = false;
  sf::Cursor cursorArrow;
  cursorArrow.loadFromSystem(sf::Cursor::Arrow);
  sf::Cursor cursorHand;
  cursorHand.loadFromSystem(sf::Cursor::Hand);
  sf::Cursor cursorCross;
  cursorCross.loadFromSystem(sf::Cursor::Cross);
  sf::Cursor cursorSizeAll;
  cursorSizeAll.loadFromSystem(sf::Cursor::SizeAll);

  auto createFigure = [&](ui::Tool tool, float width,
                          float height) -> std::unique_ptr<core::Figure> {
    width = std::max(width, 50.f);
    height = std::max(height, 50.f);

    std::unique_ptr<core::Figure> fig;
    if (tool == ui::Tool::Rectangle)
      fig = std::make_unique<core::Rectangle>(width, height);
    else if (tool == ui::Tool::Triangle)
      fig = std::make_unique<core::Triangle>(width, height);
    else if (tool == ui::Tool::Hexagon)
      fig = std::make_unique<core::Hexagon>(std::min(width, height) / 2.f);
    else if (tool == ui::Tool::Rhombus)
      fig = std::make_unique<core::Rhombus>(width, height);
    else if (tool == ui::Tool::Trapezoid)
      fig = std::make_unique<core::Trapezoid>(width * 0.6f, width, height);

    if (fig) {
      fig->fillColor = sf::Color(150, 150, 150);
      fig->edges.push_back({2.f, sf::Color::Black});
    }
    return fig;
  };

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(window, event);

      if (event.type == sf::Event::Closed) {
        window.close();
      }

      ImGuiIO &io = ImGui::GetIO();

      // Keyboard Shortcuts
      if (event.type == sf::Event::KeyPressed && !io.WantCaptureKeyboard) {
        if (event.key.code == sf::Keyboard::V)
          currentTool = ui::Tool::Select;
        else if (event.key.code == sf::Keyboard::R)
          currentTool = ui::Tool::Rectangle;
        else if (event.key.code == sf::Keyboard::T)
          currentTool = ui::Tool::Triangle;
        else if (event.key.code == sf::Keyboard::H)
          currentTool = ui::Tool::Hexagon;
        else if (event.key.code == sf::Keyboard::D)
          currentTool = ui::Tool::Rhombus;
        else if (event.key.code == sf::Keyboard::Z)
          currentTool = ui::Tool::Trapezoid;
        else if (event.key.code == sf::Keyboard::Escape) {
          scene.setSelectedFigure(nullptr);
          if (isCreating)
            isCreating = false;
        } else if (event.key.code == sf::Keyboard::G) {
          showGrid = !showGrid;
        } else if (event.key.code == sf::Keyboard::Delete ||
                   event.key.code == sf::Keyboard::Backspace) {
          if (scene.getSelectedFigure()) {
            scene.removeFigure(scene.getSelectedFigure());
            scene.setSelectedFigure(nullptr);
          }
        }
      }

      // Mouse Events
      if (!io.WantCaptureMouse) {
        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
          sf::Vector2f mousePos =
              window.mapPixelToCoords(sf::Mouse::getPosition(window));

          if (currentTool == ui::Tool::Select) {
            core::Figure *hit = scene.hitTest(mousePos);
            scene.setSelectedFigure(hit);
            if (hit) {
              isDragging = true;
              dragOffset = mousePos - hit->anchor;
            }
          } else {
            isCreating = true;
            createStartPos = mousePos;
            scene.setSelectedFigure(nullptr);
          }
        } else if (event.type == sf::Event::MouseButtonReleased &&
                   event.mouseButton.button == sf::Mouse::Left) {
          if (isDragging) {
            isDragging = false;
          }
          if (isCreating) {
            isCreating = false;
            sf::Vector2f mousePos =
                window.mapPixelToCoords(sf::Mouse::getPosition(window));

            float width = std::abs(mousePos.x - createStartPos.x);
            float height = std::abs(mousePos.y - createStartPos.y);

            // Default size if just clicked
            if (width < 5 && height < 5) {
              width = 100.f;
              height = 100.f;
            }

            sf::Vector2f center = (createStartPos + mousePos) / 2.f;
            // If simple click, use click pos
            if (std::abs(mousePos.x - createStartPos.x) < 5 &&
                std::abs(mousePos.y - createStartPos.y) < 5) {
              center = mousePos;
            }

            auto fig = createFigure(currentTool, width, height);
            if (fig) {
              fig->anchor = center;
              scene.setSelectedFigure(fig.get());
              scene.addFigure(std::move(fig));
            }
            currentTool =
                ui::Tool::Select; // Automatically switch back to select
          }
        } else if (event.type == sf::Event::MouseMoved) {
          if (isDragging && scene.getSelectedFigure()) {
            sf::Vector2f mousePos =
                window.mapPixelToCoords(sf::Mouse::getPosition(window));
            scene.getSelectedFigure()->anchor = mousePos - dragOffset;
          }
        }
      }
      // Update Cursors
      if (!io.WantCaptureMouse) {
        sf::Vector2f mousePos =
            window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (currentTool != ui::Tool::Select) {
          window.setMouseCursor(cursorCross);
        } else if (isDragging) {
          window.setMouseCursor(cursorSizeAll);
        } else if (scene.hitTest(mousePos) != nullptr) {
          window.setMouseCursor(cursorHand);
        } else {
          window.setMouseCursor(cursorArrow);
        }
      } else {
        window.setMouseCursor(cursorArrow);
      }

      ImGui::SFML::Update(window, deltaClock.restart());

      // Render UI
      toolbar.render(currentTool);
      propertiesPanel.render(scene.getSelectedFigure());

      window.clear(sf::Color(240, 240, 240));

      // Draw Grid
      if (showGrid) {
        sf::VertexArray grid(sf::Lines);
        sf::Color gridColor(200, 200, 200);
        int gridSize = 50;
        sf::Vector2u size = window.getSize();
        for (int x = 0; x < size.x; x += gridSize) {
          grid.append(sf::Vertex(sf::Vector2f(x, 0), gridColor));
          grid.append(sf::Vertex(sf::Vector2f(x, size.y), gridColor));
        }
        for (int y = 0; y < size.y; y += gridSize) {
          grid.append(sf::Vertex(sf::Vector2f(0, y), gridColor));
          grid.append(sf::Vertex(sf::Vector2f(size.x, y), gridColor));
        }
        window.draw(grid);
      }

      scene.drawAll(window);

      // Draw preview box if creating
      if (isCreating && currentTool != ui::Tool::Select) {
        sf::Vector2f mousePos =
            window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::RectangleShape preview;
        preview.setPosition(std::min(createStartPos.x, mousePos.x),
                            std::min(createStartPos.y, mousePos.y));
        preview.setSize(sf::Vector2f(std::abs(mousePos.x - createStartPos.x),
                                     std::abs(mousePos.y - createStartPos.y)));
        preview.setFillColor(sf::Color(150, 150, 150, 100)); // Semi-transparent
        preview.setOutlineColor(sf::Color(100, 100, 100, 200));
        preview.setOutlineThickness(1.f);
        window.draw(preview);
      }

      ImGui::SFML::Render(window);
      window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
  }
