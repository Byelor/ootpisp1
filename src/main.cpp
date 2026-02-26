#include "core/Figures.hpp"
#include "core/Scene.hpp"
#include "core/Viewport.hpp"
#include "ui/PropertiesPanel.hpp"
#include "ui/Toolbar.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <memory>

void drawAnchorMarker(sf::RenderTarget &target, sf::Vector2f pos) {
  sf::CircleShape circle(5.f);
  circle.setFillColor(sf::Color::Transparent);
  circle.setOutlineColor(sf::Color::White);
  circle.setOutlineThickness(1.5f);
  circle.setOrigin(5.f, 5.f);
  circle.setPosition(pos);

  sf::VertexArray lines(sf::Lines, 4);
  lines[0] = sf::Vertex(pos - sf::Vector2f(8.f, 0.f), sf::Color::White);
  lines[1] = sf::Vertex(pos + sf::Vector2f(8.f, 0.f), sf::Color::White);
  lines[2] = sf::Vertex(pos - sf::Vector2f(0.f, 8.f), sf::Color::White);
  lines[3] = sf::Vertex(pos + sf::Vector2f(0.f, 8.f), sf::Color::White);

  target.draw(circle);
  target.draw(lines);
}

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
  bool isDraggingAnchor = false;
  bool isNodeEditMode = false;
  bool isRotating = false;
  float rotationStartAngle = 0.f;
  float initialRotation = 0.f;

  enum class ScaleHandle { None, TL, TC, TR, CR, BR, BC, BL, CL };
  ScaleHandle hoveringScaleHandle = ScaleHandle::None;
  ScaleHandle draggingScaleHandle = ScaleHandle::None;
  sf::Vector2f scaleStartMouse;
  sf::Vector2f scaleStartValue;
  sf::Vector2f scaleStartAnchor;

  int draggingVertexIndex = -1;
  bool isCreating = false;
  int creatingStep =
      0; // 0 = not creating, 1 = first click placed, 2 = second click placed
  sf::Vector2f dragOffset;

  core::Viewport viewport;
  bool isPanning = false;
  sf::Vector2f panStartOrigin;
  sf::Vector2f panStartMouse;

  sf::Clock clickClock;
  bool wasClicked = false;
  sf::Vector2f createStartPos;

  bool showGrid = false;
  bool showOriginAxes = true;
  sf::Cursor cursorArrow;
  cursorArrow.loadFromSystem(sf::Cursor::Arrow);
  sf::Cursor cursorHand;
  cursorHand.loadFromSystem(sf::Cursor::Hand);
  sf::Cursor cursorCross;
  cursorCross.loadFromSystem(sf::Cursor::Cross);
  sf::Cursor cursorSizeAll;
  cursorSizeAll.loadFromSystem(sf::Cursor::SizeAll);

  sf::Cursor cursorSizeNWSE;
  cursorSizeNWSE.loadFromSystem(sf::Cursor::SizeTopLeftBottomRight);
  sf::Cursor cursorSizeNESW;
  cursorSizeNESW.loadFromSystem(sf::Cursor::SizeBottomLeftTopRight);
  sf::Cursor cursorSizeWE;
  cursorSizeWE.loadFromSystem(sf::Cursor::SizeHorizontal);
  sf::Cursor cursorSizeNS;
  cursorSizeNS.loadFromSystem(sf::Cursor::SizeVertical);

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
      for (auto &edge : fig->edges) {
        edge.width = 2.f;
        edge.color = sf::Color::Black;
      }
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
        bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
        if (ctrl) {
          if (event.key.code == sf::Keyboard::Equal ||
              event.key.code == sf::Keyboard::Add) {
            sf::Vector2f center(window.getSize().x / 2.f,
                                window.getSize().y / 2.f);
            viewport.zoomAt(center, 1.1f);
          } else if (event.key.code == sf::Keyboard::Hyphen ||
                     event.key.code == sf::Keyboard::Subtract) {
            sf::Vector2f center(window.getSize().x / 2.f,
                                window.getSize().y / 2.f);
            viewport.zoomAt(center, 1.f / 1.1f);
          } else if (event.key.code == sf::Keyboard::Num0 ||
                     event.key.code == sf::Keyboard::Numpad0) {
            sf::Vector2f center(window.getSize().x / 2.f,
                                window.getSize().y / 2.f);
            viewport.zoomAt(center, 1.f / viewport.zoom); // Reset to 100%
          }
        } else if (event.key.code == sf::Keyboard::V)
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
          if (isNodeEditMode) {
            isNodeEditMode = false;
          } else {
            scene.setSelectedFigure(nullptr);
            if (isCreating) {
              isCreating = false;
              creatingStep = 0;
            }
          }
        } else if (event.key.code == sf::Keyboard::N) {
          if (scene.getSelectedFigure()) {
            isNodeEditMode = !isNodeEditMode;
          }
        } else if (event.key.code == sf::Keyboard::G) {
          showGrid = !showGrid;
        } else if (event.key.code == sf::Keyboard::A) {
          showOriginAxes = !showOriginAxes;
        } else if (event.key.code == sf::Keyboard::Delete ||
                   event.key.code == sf::Keyboard::Backspace) {
          if (scene.getSelectedFigure()) {
            scene.removeFigure(scene.getSelectedFigure());
            scene.setSelectedFigure(nullptr);
          }
        }
      }

      // Mouse Wheel Zoom
      if (event.type == sf::Event::MouseWheelScrolled && !io.WantCaptureMouse) {
        if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
          float zoomFactor = std::pow(1.1f, event.mouseWheelScroll.delta);
          sf::Vector2f mousePosScreen(event.mouseWheelScroll.x,
                                      event.mouseWheelScroll.y);
          viewport.zoomAt(mousePosScreen, zoomFactor);
        }
      }

      // Mouse Events
      if (!io.WantCaptureMouse) {
        if (event.type == sf::Event::MouseButtonPressed) {
          if (event.mouseButton.button == sf::Mouse::Middle ||
              (event.mouseButton.button == sf::Mouse::Left &&
               sf::Keyboard::isKeyPressed(sf::Keyboard::Space))) {
            isPanning = true;
            panStartMouse =
                sf::Vector2f(event.mouseButton.x, event.mouseButton.y);
            panStartOrigin = viewport.worldOrigin;
          } else if (event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseButton.x, event.mouseButton.y));

            if (currentTool == ui::Tool::Select) {
              bool doubleClicked = false;
              if (wasClicked &&
                  clickClock.getElapsedTime().asSeconds() < 0.3f) {
                doubleClicked = true;
                wasClicked = false;
              } else {
                wasClicked = true;
                clickClock.restart();
              }

              bool altPressed =
                  sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

              core::Figure *selFig = scene.getSelectedFigure();
              bool hitAnchor = false;
              bool hitRotationMarker = false;
              int hoveredVertex = -1;

              if (selFig) {
                float dist = std::hypot(mousePos.x - selFig->anchor.x,
                                        mousePos.y - selFig->anchor.y);
                if (dist <= 10.f) {
                  hitAnchor = true;
                }

                sf::FloatRect localBounds = selFig->getLocalBoundingBox();
                sf::Vector2f tl(localBounds.left, localBounds.top);
                sf::Vector2f tr(localBounds.left + localBounds.width,
                                localBounds.top);
                sf::Vector2f tc = (tl + tr) / 2.f;
                sf::Vector2f absTc = selFig->getAbsoluteVertex(tc);
                float rotRad = selFig->rotationAngle * M_PI / 180.f;
                sf::Vector2f rotOffset(std::sin(rotRad) * 20.f,
                                       -std::cos(rotRad) * 20.f);
                sf::Vector2f rotMarker = absTc + rotOffset;

                if (std::hypot(mousePos.x - rotMarker.x,
                               mousePos.y - rotMarker.y) <= 8.f) {
                  hitRotationMarker = true;
                }

                if (isNodeEditMode) {
                  const auto &verts = selFig->getVertices();
                  for (size_t i = 0; i < verts.size(); ++i) {
                    sf::Vector2f absV = selFig->getAbsoluteVertex(verts[i]);
                    if (std::abs(mousePos.x - absV.x) <= 6.f &&
                        std::abs(mousePos.y - absV.y) <= 6.f) {
                      hoveredVertex = i;
                      break;
                    }
                  }
                }
              }

              if (hoveringScaleHandle != ScaleHandle::None && selFig) {
                draggingScaleHandle = hoveringScaleHandle;
                scaleStartMouse = mousePos;
                scaleStartValue = selFig->scale;
                scaleStartAnchor = selFig->anchor;
              } else if (hitRotationMarker && selFig) {
                isRotating = true;
                rotationStartAngle = std::atan2(mousePos.y - selFig->anchor.y,
                                                mousePos.x - selFig->anchor.x) *
                                     180.f / M_PI;
                initialRotation = selFig->rotationAngle;
              } else if (isNodeEditMode && hoveredVertex != -1) {
                draggingVertexIndex = hoveredVertex;
              } else if (hitAnchor && altPressed && selFig) {
                isDraggingAnchor = true;
                dragOffset = mousePos - selFig->anchor;
              } else {
                core::Figure *hit = scene.hitTest(mousePos);
                scene.setSelectedFigure(hit);
                if (selFig != hit)
                  isNodeEditMode = false;

                if (hit && doubleClicked) {
                  isNodeEditMode = true;
                } else if (hit) {
                  isDragging = true;
                  dragOffset = mousePos - hit->anchor;
                }
              }
            } else {
              // Creating mode: two-clicks
              if (creatingStep == 0) {
                isCreating = true;
                creatingStep = 1;
                createStartPos = mousePos;
                scene.setSelectedFigure(nullptr);
                std::cout << "Step 1: First click to create figure at "
                          << createStartPos.x << ", " << createStartPos.y
                          << std::endl;
              } else if (creatingStep == 1) {
                creatingStep = 2; // Second click
              }
            }
          }
        } else if (event.type == sf::Event::MouseButtonReleased) {
          if (event.mouseButton.button == sf::Mouse::Middle ||
              (event.mouseButton.button == sf::Mouse::Left && isPanning)) {
            isPanning = false;
          }
          if (event.mouseButton.button == sf::Mouse::Left) {
            if (draggingScaleHandle != ScaleHandle::None) {
              draggingScaleHandle = ScaleHandle::None;
            }
            if (isDragging) {
              isDragging = false;
            }
            if (isDraggingAnchor) {
              isDraggingAnchor = false;
            }
            if (isRotating) {
              isRotating = false;
            }
            if (draggingVertexIndex != -1) {
              draggingVertexIndex = -1;
            }
            if (isCreating && creatingStep == 2) {
              isCreating = false;
              creatingStep = 0;
              sf::Vector2f mousePos = viewport.screenToWorld(
                  sf::Vector2f(event.mouseButton.x, event.mouseButton.y));

              float width = std::abs(mousePos.x - createStartPos.x);
              float height = std::abs(mousePos.y - createStartPos.y);

              // Default size if very small
              if (width < 5 && height < 5) {
                width = 100.f;
                height = 100.f;
              }

              sf::Vector2f center = (createStartPos + mousePos) / 2.f;

              std::cout << "Creating figure of type: "
                        << static_cast<int>(currentTool)
                        << " with width: " << width << ", height: " << height
                        << std::endl;

              auto fig = createFigure(currentTool, width, height);
              if (fig) {
                fig->anchor = center;
                scene.setSelectedFigure(fig.get());
                scene.addFigure(std::move(fig));
                std::cout
                    << "Figure successfully added to scene. Total figures: "
                    << scene.getFigures().size() << std::endl;
              } else {
                std::cout << "createFigure returned nullptr!" << std::endl;
              }
              currentTool =
                  ui::Tool::Select; // Automatically switch back to select
            }
          }
        } else if (event.type == sf::Event::MouseMoved) {
          sf::Vector2f mousePos = viewport.screenToWorld(
              sf::Vector2f(event.mouseMove.x, event.mouseMove.y));

          if (isPanning) {
            sf::Vector2f currentMouse(event.mouseMove.x, event.mouseMove.y);
            sf::Vector2f delta = currentMouse - panStartMouse;
            viewport.worldOrigin = panStartOrigin + delta;
          } else if (draggingScaleHandle != ScaleHandle::None &&
                     scene.getSelectedFigure()) {
            core::Figure *selFig = scene.getSelectedFigure();
            sf::Vector2f delta = mousePos - scaleStartMouse;
            float rad = -selFig->rotationAngle * M_PI / 180.f;
            float dx = delta.x * std::cos(rad) - delta.y * std::sin(rad);
            float dy = delta.x * std::sin(rad) + delta.y * std::cos(rad);

            sf::FloatRect localBounds = selFig->getLocalBoundingBox();
            sf::Vector2f newScale = scaleStartValue;
            bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

            float curAbsWidth = localBounds.width * std::abs(scaleStartValue.x);
            float curAbsHeight =
                localBounds.height * std::abs(scaleStartValue.y);
            if (curAbsWidth < 0.001f)
              curAbsWidth = 0.001f;
            if (curAbsHeight < 0.001f)
              curAbsHeight = 0.001f;

            float scaleX_mult = 1.0f;
            float scaleY_mult = 1.0f;

            if (draggingScaleHandle == ScaleHandle::TR ||
                draggingScaleHandle == ScaleHandle::CR ||
                draggingScaleHandle == ScaleHandle::BR) {
              scaleX_mult = (curAbsWidth + dx) / curAbsWidth;
            } else if (draggingScaleHandle == ScaleHandle::TL ||
                       draggingScaleHandle == ScaleHandle::CL ||
                       draggingScaleHandle == ScaleHandle::BL) {
              scaleX_mult = (curAbsWidth - dx) / curAbsWidth;
            }

            if (draggingScaleHandle == ScaleHandle::TR ||
                draggingScaleHandle == ScaleHandle::TC ||
                draggingScaleHandle == ScaleHandle::TL) {
              scaleY_mult = (curAbsHeight - dy) / curAbsHeight;
            } else if (draggingScaleHandle == ScaleHandle::BR ||
                       draggingScaleHandle == ScaleHandle::BC ||
                       draggingScaleHandle == ScaleHandle::BL) {
              scaleY_mult = (curAbsHeight + dy) / curAbsHeight;
            }

            bool isCorner = (draggingScaleHandle == ScaleHandle::TL ||
                             draggingScaleHandle == ScaleHandle::TR ||
                             draggingScaleHandle == ScaleHandle::BL ||
                             draggingScaleHandle == ScaleHandle::BR);
            if (shift && isCorner) {
              float maxMult =
                  std::max(std::abs(scaleX_mult), std::abs(scaleY_mult));
              float signX = scaleX_mult < 0 ? -1 : 1;
              float signY = scaleY_mult < 0 ? -1 : 1;
              scaleX_mult = maxMult * signX;
              scaleY_mult = maxMult * signY;
            }

            newScale.x = scaleStartValue.x * scaleX_mult;
            newScale.y = scaleStartValue.y * scaleY_mult;

            if (std::abs(newScale.x) < 0.01f)
              newScale.x = 0.01f * (newScale.x < 0 ? -1 : 1);
            if (std::abs(newScale.y) < 0.01f)
              newScale.y = 0.01f * (newScale.y < 0 ? -1 : 1);

            selFig->scale = newScale;

            // Adjust anchor to keep the opposite boundary invariant
            sf::Vector2f v_inv(0.f, 0.f);
            if (draggingScaleHandle == ScaleHandle::TL ||
                draggingScaleHandle == ScaleHandle::CL ||
                draggingScaleHandle == ScaleHandle::BL) {
              v_inv.x = localBounds.left + localBounds.width;
            } else if (draggingScaleHandle == ScaleHandle::TR ||
                       draggingScaleHandle == ScaleHandle::CR ||
                       draggingScaleHandle == ScaleHandle::BR) {
              v_inv.x = localBounds.left;
            }

            if (draggingScaleHandle == ScaleHandle::TL ||
                draggingScaleHandle == ScaleHandle::TC ||
                draggingScaleHandle == ScaleHandle::TR) {
              v_inv.y = localBounds.top + localBounds.height;
            } else if (draggingScaleHandle == ScaleHandle::BL ||
                       draggingScaleHandle == ScaleHandle::BC ||
                       draggingScaleHandle == ScaleHandle::BR) {
              v_inv.y = localBounds.top;
            }

            sf::Vector2f V_old(v_inv.x * scaleStartValue.x,
                               v_inv.y * scaleStartValue.y);
            sf::Vector2f V_new(v_inv.x * newScale.x, v_inv.y * newScale.y);
            sf::Vector2f delta_V = V_old - V_new;

            float rad2 = selFig->rotationAngle * M_PI / 180.f;
            float anchor_dx =
                delta_V.x * std::cos(rad2) - delta_V.y * std::sin(rad2);
            float anchor_dy =
                delta_V.x * std::sin(rad2) + delta_V.y * std::cos(rad2);

            selFig->anchor =
                scaleStartAnchor + sf::Vector2f(anchor_dx, anchor_dy);
          } else if (isRotating && scene.getSelectedFigure()) {
            float currentAngle =
                std::atan2(mousePos.y - scene.getSelectedFigure()->anchor.y,
                           mousePos.x - scene.getSelectedFigure()->anchor.x) *
                180.f / M_PI;
            float delta = currentAngle - rotationStartAngle;
            float newRot = initialRotation + delta;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
              newRot = std::round(newRot / 15.f) * 15.f;
            }
            scene.getSelectedFigure()->rotationAngle = newRot;
          } else if (draggingVertexIndex != -1 && scene.getSelectedFigure()) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            auto &verts = scene.getSelectedFigure()->getVerticesMutable();

            sf::Vector2f deltaAbs =
                mousePos - scene.getSelectedFigure()->anchor;
            float invRad =
                -scene.getSelectedFigure()->rotationAngle * M_PI / 180.f;
            float rx =
                deltaAbs.x * std::cos(invRad) - deltaAbs.y * std::sin(invRad);
            float ry =
                deltaAbs.x * std::sin(invRad) + deltaAbs.y * std::cos(invRad);

            float scaledX = rx / scene.getSelectedFigure()->scale.x;
            float scaledY = ry / scene.getSelectedFigure()->scale.y;
            verts[draggingVertexIndex] = sf::Vector2f(scaledX, scaledY);
          } else if (isDraggingAnchor && scene.getSelectedFigure()) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            sf::Vector2f newAnchor = mousePos - dragOffset;
            sf::Vector2f delta = newAnchor - scene.getSelectedFigure()->anchor;

            scene.getSelectedFigure()->anchor = newAnchor;
            for (auto &v : scene.getSelectedFigure()->getVerticesMutable()) {
              v -= delta;
            }
          } else if (isDragging && scene.getSelectedFigure()) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            scene.getSelectedFigure()->anchor = mousePos - dragOffset;
          }
        }
      }
    }

    // Update Cursors and Hover
    ScaleHandle newHoverHandle = ScaleHandle::None;
    if (!isNodeEditMode && scene.getSelectedFigure()) {
      auto selFig = scene.getSelectedFigure();
      sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(
          sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
      sf::FloatRect localBounds = selFig->getLocalBoundingBox();
      sf::Vector2f tl(localBounds.left, localBounds.top);
      sf::Vector2f tr(localBounds.left + localBounds.width, localBounds.top);
      sf::Vector2f bl(localBounds.left, localBounds.top + localBounds.height);
      sf::Vector2f br(localBounds.left + localBounds.width,
                      localBounds.top + localBounds.height);
      sf::Vector2f tc = (tl + tr) / 2.f;
      sf::Vector2f bc = (bl + br) / 2.f;
      sf::Vector2f cl = (tl + bl) / 2.f;
      sf::Vector2f cr = (tr + br) / 2.f;
      std::vector<sf::Vector2f> handles = {tl, tc, tr, cr, br, bc, bl, cl};
      for (int i = 0; i < 8; ++i) {
        sf::Vector2f absH = selFig->getAbsoluteVertex(handles[i]);
        if (std::hypot(mousePos.x - absH.x, mousePos.y - absH.y) <= 8.f) {
          newHoverHandle = static_cast<ScaleHandle>(i + 1);
          break;
        }
      }
    }
    hoveringScaleHandle = newHoverHandle;

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
      sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(
          sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));

      ScaleHandle activeHandle = draggingScaleHandle != ScaleHandle::None
                                     ? draggingScaleHandle
                                     : hoveringScaleHandle;

      if (isPanning || (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) &&
                        currentTool == ui::Tool::Select)) {
        window.setMouseCursor(cursorHand);
      } else if (activeHandle != ScaleHandle::None) {
        if (activeHandle == ScaleHandle::TL || activeHandle == ScaleHandle::BR)
          window.setMouseCursor(cursorSizeNWSE);
        else if (activeHandle == ScaleHandle::TR ||
                 activeHandle == ScaleHandle::BL)
          window.setMouseCursor(cursorSizeNESW);
        else if (activeHandle == ScaleHandle::TC ||
                 activeHandle == ScaleHandle::BC)
          window.setMouseCursor(cursorSizeNS);
        else if (activeHandle == ScaleHandle::CL ||
                 activeHandle == ScaleHandle::CR)
          window.setMouseCursor(cursorSizeWE);
      } else if (currentTool != ui::Tool::Select) {
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
    bool fitRequested =
        propertiesPanel.render(scene.getSelectedFigure(), viewport);

    if (fitRequested && !scene.getFigures().empty()) {
      const auto &figs = scene.getFigures();
      sf::FloatRect first = figs.front()->getBoundingBox();
      float minX = first.left, minY = first.top,
            maxX = first.left + first.width, maxY = first.top + first.height;
      for (size_t i = 1; i < figs.size(); ++i) {
        sf::FloatRect b = figs[i]->getBoundingBox();
        minX = std::min(minX, b.left);
        minY = std::min(minY, b.top);
        maxX = std::max(maxX, b.left + b.width);
        maxY = std::max(maxY, b.top + b.height);
      }

      float margin = 50.f;
      float width = maxX - minX + margin * 2;
      float height = maxY - minY + margin * 2;

      float zoomX = window.getSize().x / width;
      float zoomY = window.getSize().y / height;
      viewport.zoom = std::clamp(std::min(zoomX, zoomY), 0.05f, 50.f);

      sf::Vector2f centerWorld(minX + (maxX - minX) / 2.f,
                               minY + (maxY - minY) / 2.f);
      sf::Vector2f centerScreen(window.getSize().x / 2.f,
                                window.getSize().y / 2.f);

      viewport.worldOrigin = centerScreen - centerWorld * viewport.zoom;
    }

    // Status Bar
    ImGui::SetNextWindowPos(ImVec2(0, window.getSize().y - 30),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(window.getSize().x - 280, 30),
                             ImGuiCond_Always);
    ImGuiWindowFlags statusFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));
    ImGui::Begin("Status Bar", nullptr, statusFlags);
    sf::Vector2f mPos = viewport.screenToWorld(sf::Vector2f(
        sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
    ImGui::Text("Zoom: %.0f%%   |   Cursor: (%.1f, %.1f)",
                viewport.zoom * 100.f, mPos.x, mPos.y);
    ImGui::End();
    ImGui::PopStyleVar();

    window.clear(sf::Color(240, 240, 240));

    sf::View oldView = window.getView();
    window.setView(viewport.getView(sf::Vector2f(window.getSize())));

    // Draw Grid
    if (showGrid) {
      sf::VertexArray grid(sf::Lines);
      sf::Color gridColor(200, 200, 200);
      int gridSize = 50;

      sf::Vector2f viewCenter = window.getView().getCenter();
      sf::Vector2f viewSize = window.getView().getSize();

      float startX =
          std::floor((viewCenter.x - viewSize.x / 2.f) / gridSize) * gridSize;
      float endX =
          std::floor((viewCenter.x + viewSize.x / 2.f) / gridSize) * gridSize +
          gridSize;

      float startY =
          std::floor((viewCenter.y - viewSize.y / 2.f) / gridSize) * gridSize;
      float endY =
          std::floor((viewCenter.y + viewSize.y / 2.f) / gridSize) * gridSize +
          gridSize;

      for (float x = startX; x <= endX; x += gridSize) {
        grid.append(sf::Vertex(sf::Vector2f(x, startY), gridColor));
        grid.append(sf::Vertex(sf::Vector2f(x, endY), gridColor));
      }
      for (float y = startY; y <= endY; y += gridSize) {
        grid.append(sf::Vertex(sf::Vector2f(startX, y), gridColor));
        grid.append(sf::Vertex(sf::Vector2f(endX, y), gridColor));
      }
      window.draw(grid);
    }

    // Global Origin Indicator
    if (showOriginAxes) {
      sf::VertexArray originAxes(sf::Lines, 4);
      sf::Color axisColor(100, 100, 100, 150);
      originAxes[0] = sf::Vertex(sf::Vector2f(-20.f, 0.f), axisColor);
      originAxes[1] = sf::Vertex(sf::Vector2f(20.f, 0.f), axisColor);
      originAxes[2] = sf::Vertex(sf::Vector2f(0.f, -20.f), axisColor);
      originAxes[3] = sf::Vertex(sf::Vector2f(0.f, 20.f), axisColor);
      window.draw(originAxes);
    }

    scene.drawAll(window);

    // Draw Anchor Marker if figure is selected
    if (scene.getSelectedFigure()) {
      drawAnchorMarker(window, scene.getSelectedFigure()->anchor);

      sf::FloatRect localBounds =
          scene.getSelectedFigure()->getLocalBoundingBox();
      sf::Vector2f tl(localBounds.left, localBounds.top);
      sf::Vector2f tr(localBounds.left + localBounds.width, localBounds.top);
      sf::Vector2f bl(localBounds.left, localBounds.top + localBounds.height);
      sf::Vector2f br(localBounds.left + localBounds.width,
                      localBounds.top + localBounds.height);
      sf::Vector2f tc = (tl + tr) / 2.f;
      sf::Vector2f bc = (bl + br) / 2.f;
      sf::Vector2f cl = (tl + bl) / 2.f;
      sf::Vector2f cr = (tr + br) / 2.f;

      std::vector<sf::Vector2f> handles = {tl, tc, tr, cr, br, bc, bl, cl};

      if (!isNodeEditMode) {
        for (const auto &h : handles) {
          sf::RectangleShape handle(sf::Vector2f(8.f, 8.f));
          handle.setOrigin(4.f, 4.f);
          handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(h));
          handle.setRotation(scene.getSelectedFigure()->rotationAngle);
          handle.setFillColor(sf::Color::White);
          handle.setOutlineColor(sf::Color(0, 120, 215));
          handle.setOutlineThickness(1.5f);
          window.draw(handle);
        }
      }

      sf::Vector2f absTc = scene.getSelectedFigure()->getAbsoluteVertex(tc);
      float rotRad = scene.getSelectedFigure()->rotationAngle * M_PI / 180.f;
      sf::Vector2f rotOffset(std::sin(rotRad) * 20.f, -std::cos(rotRad) * 20.f);
      sf::Vector2f rotPos = absTc + rotOffset;

      sf::CircleShape rotMarker(5.f);
      rotMarker.setOrigin(5.f, 5.f);
      rotMarker.setPosition(rotPos);
      rotMarker.setFillColor(sf::Color::White);
      rotMarker.setOutlineColor(sf::Color(0, 120, 215));
      rotMarker.setOutlineThickness(1.5f);
      window.draw(rotMarker);

      sf::VertexArray rotLine(sf::Lines, 2);
      rotLine[0] = sf::Vertex(absTc, sf::Color(0, 120, 215));
      rotLine[1] = sf::Vertex(rotPos, sf::Color(0, 120, 215));
      window.draw(rotLine);

      if (isNodeEditMode) {
        const auto &verts = scene.getSelectedFigure()->getVertices();
        for (const auto &v : verts) {
          sf::RectangleShape handle(sf::Vector2f(8.f, 8.f));
          handle.setOrigin(4.f, 4.f);
          handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(v));
          handle.setFillColor(sf::Color::White);
          handle.setOutlineColor(sf::Color(0, 120, 215));
          handle.setOutlineThickness(1.5f);
          window.draw(handle);
        }
      }
    }

    // Draw preview box if creating
    if (isCreating && creatingStep == 1 && currentTool != ui::Tool::Select) {
      sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(
          sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
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

    window.setView(oldView);
    ImGui::SFML::Render(window);
    window.display();
  }

  ImGui::SFML::Shutdown();
  return 0;
}
