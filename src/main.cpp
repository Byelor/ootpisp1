#include "core/CompositeFigure.hpp"
#include "core/Figures.hpp"
#include "core/PolylineFigure.hpp"
#include "core/Scene.hpp"
#include "core/Viewport.hpp"
#include "core/MathUtils.hpp"
#include "ui/Toolbar.hpp"
#include "ui/PropertiesPanel.hpp"
#include "ui/CreateFigureModal.hpp"
#include "ui/LayerPanel.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cmath>
#include <imgui-SFML.h>
#include <imgui.h>
#include <iostream>
#include <memory>

using namespace core;
using namespace core::math;

void drawAnchorMarker(sf::RenderTarget &target, sf::Vector2f pos,
                      float markerScale) {
  float r = 5.f * markerScale;
  sf::CircleShape circle(r);
  circle.setFillColor(sf::Color::White); // Solid white inside for contrast
  circle.setOutlineColor(sf::Color(0, 120, 215)); // Blue outline
  circle.setOutlineThickness(1.5f * markerScale);
  circle.setOrigin(r, r);
  circle.setPosition(pos);

  sf::VertexArray lines(sf::Lines, 4);
  sf::Color lineColor(0, 120, 215);
  lines[0] = sf::Vertex(pos - sf::Vector2f(8.f * markerScale, 0.f), lineColor);
  lines[1] = sf::Vertex(pos + sf::Vector2f(8.f * markerScale, 0.f), lineColor);
  lines[2] = sf::Vertex(pos - sf::Vector2f(0.f, 8.f * markerScale), lineColor);
  lines[3] = sf::Vertex(pos + sf::Vector2f(0.f, 8.f * markerScale), lineColor);

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
  ui::LayerPanel layerPanel;
  ui::CreateFigureModal createModal;
  ui::Tool currentTool = ui::Tool::Select;

  bool isDragging = false;
  bool isDraggingAnchor = false;
  bool isDrilledIntoGroup = false;
  bool isDraggingSubfigure = false;       // actively moving a subfigure
  bool isDraggingSubfigurePending = false; // waiting to see if it becomes a drag
  sf::Vector2f subfigureDragStart;        // screen pos where drag initiated
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

  bool isDraggingCustomOrigin = false;

  sf::Clock clickClock;
  bool wasClicked = false;
  sf::Vector2f createStartPos;

  int selectedCustomToolId = -1;
  std::vector<sf::Vector2f> currentPolylineVertices;
  float polylineStartInput[2] = {0.f, 0.f};
  float polylineSegmentLengthInput = 100.f;
  float polylineSegmentAngleInputDeg = 0.f;
  std::vector<core::Figure*> compoundSelection;
  std::vector<std::unique_ptr<core::Figure>> userRegistry;

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
    if (tool == ui::Tool::Rectangle) {
      fig = std::make_unique<core::Rectangle>(100.f, 100.f);
      fig->scale = sf::Vector2f(width / 100.f, height / 100.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Triangle) {
      fig = std::make_unique<core::Triangle>(100.f, 100.f);
      fig->scale = sf::Vector2f(width / 100.f, height / 100.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Hexagon) {
      fig = std::make_unique<core::Hexagon>(100.f, 100.f);
      fig->scale = sf::Vector2f(width / 100.f, height / 100.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Rhombus) {
      fig = std::make_unique<core::Rhombus>(100.f, 100.f);
      fig->scale = sf::Vector2f(width / 100.f, height / 100.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Trapezoid) {
      fig = std::make_unique<core::Trapezoid>(60.f, 100.f, 100.f);
      fig->scale = sf::Vector2f(width / 100.f, height / 100.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Circle) {
      // Circle constructor takes radiusX, radiusY so divide by 2
      fig = std::make_unique<core::Circle>(width / 2.f, height / 2.f);
      fig->scale = sf::Vector2f(1.f, 1.f);
      fig->applyScale();
    } else if (tool == ui::Tool::Custom && selectedCustomToolId >= 0 && selectedCustomToolId < userRegistry.size()) {
        fig = userRegistry[selectedCustomToolId]->clone();
        sf::FloatRect bounds = fig->getLocalBoundingBox();
        if (bounds.width > 0 && bounds.height > 0) {
            fig->scale.x *= width / bounds.width;
            fig->scale.y *= height / bounds.height;
        }
    }

    if (fig && tool != ui::Tool::Custom) {
      fig->fillColor = sf::Color(150, 150, 150);
      for (auto &edge : fig->edges) {
        edge.width = 2.f;
        edge.color = sf::Color::Black;
      }
    }
    return fig;
  };

  auto finishCurrentPolyline = [&]() {
    if (currentPolylineVertices.size() < 2) {
      return;
    }
    auto fig = std::make_unique<core::PolylineFigure>();
    fig->fillColor = sf::Color(150, 150, 150, 200);
    fig->figureName = "Polygon";
    sf::Vector2f minP = currentPolylineVertices[0];
    sf::Vector2f maxP = currentPolylineVertices[0];
    for (auto &v : currentPolylineVertices) {
      minP.x = std::min(minP.x, v.x);
      minP.y = std::min(minP.y, v.y);
      maxP.x = std::max(maxP.x, v.x);
      maxP.y = std::max(maxP.y, v.y);
    }
    sf::Vector2f center = (minP + maxP) / 2.f;
    fig->anchor = center;
    std::vector<sf::Vector2f> localVerts;
    for (auto &v : currentPolylineVertices) {
      localVerts.push_back(v - center);
    }
    fig->setVertices(localVerts);
    fig->edges.resize(localVerts.size());
    for (auto &e : fig->edges) {
      e.width = 2.f;
      e.color = sf::Color::Black;
    }
    scene.setSelectedFigure(fig.get());
    scene.addFigure(std::move(fig));
    currentPolylineVertices.clear();
    currentTool = ui::Tool::Select;
  };

  // Add initial colored figures
  {
    std::vector<sf::Color> colors = {
        sf::Color(255, 100, 100), sf::Color(100, 255, 100),
        sf::Color(100, 100, 255), sf::Color(255, 255, 100),
        sf::Color(255, 100, 255), sf::Color(100, 255, 255)};
    std::vector<ui::Tool> tempTools = {ui::Tool::Rectangle, ui::Tool::Triangle,
                                       ui::Tool::Hexagon,   ui::Tool::Rhombus,
                                       ui::Tool::Trapezoid, ui::Tool::Circle};

    for (size_t i = 0; i < tempTools.size(); ++i) {
      auto fig = createFigure(tempTools[i], 150.f, 150.f);
      fig->fillColor = colors[i];
      // Give edges varied colors
      for (size_t j = 0; j < fig->edges.size(); ++j) {
        fig->edges[j].color = colors[(i + j + 1) % colors.size()];
        fig->edges[j].width = 4.f;
      }
      fig->anchor =
          sf::Vector2f(250.f + (i % 3) * 250.f, 250.f + (i / 3) * 250.f);
      scene.addFigure(std::move(fig));
    }
  }

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
        else if (event.key.code == sf::Keyboard::C)
          currentTool = ui::Tool::Circle;
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
        } else if (event.key.code == sf::Keyboard::Up ||
                   event.key.code == sf::Keyboard::Down ||
                   event.key.code == sf::Keyboard::Left ||
                   event.key.code == sf::Keyboard::Right) {
          if (core::Figure *fig = scene.getSelectedFigure()) {
            float moveAmt =
                sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)
                    ? 10.f
                    : 1.f;
            if (event.key.code == sf::Keyboard::Up)
              fig->move(sf::Vector2f(0.f, -moveAmt));
            else if (event.key.code == sf::Keyboard::Down)
              fig->move(sf::Vector2f(0.f, moveAmt));
            else if (event.key.code == sf::Keyboard::Left)
              fig->move(sf::Vector2f(-moveAmt, 0.f));
            else if (event.key.code == sf::Keyboard::Right)
              fig->move(sf::Vector2f(moveAmt, 0.f));
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
          } else if (event.mouseButton.button == sf::Mouse::Right) {
            if (currentTool == ui::Tool::Polyline && currentPolylineVertices.size() > 1) {
                finishCurrentPolyline();
            } else {
                sf::Vector2f mousePosScreen(event.mouseButton.x,
                                            event.mouseButton.y);
                sf::Vector2f mousePos = viewport.screenToWorld(mousePosScreen);
                createModal.open(mousePos); // always open on right-click
            }
          } else if (event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePosScreen(event.mouseButton.x,
                                        event.mouseButton.y);
            sf::Vector2f mousePos = viewport.screenToWorld(mousePosScreen);

            // Check if clicking custom origin marker
            if (scene.customOriginActive) {
              sf::Vector2f customScreen =
                  viewport.worldToScreen(scene.customOriginPos);
              if (std::hypot(mousePosScreen.x - customScreen.x,
                             mousePosScreen.y - customScreen.y) <= 15.f) {
                bool doubleClickedOrigin = false;
                if (wasClicked &&
                    clickClock.getElapsedTime().asSeconds() < 0.3f) {
                  doubleClickedOrigin = true;
                  wasClicked = false;
                } else {
                  wasClicked = true;
                  clickClock.restart();
                }

                if (doubleClickedOrigin) {
                  scene.resetCustomOrigin();
                } else {
                  isDraggingCustomOrigin = true;
                }
                continue;
              }
            }

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

              if (doubleClicked && !scene.hitTest(mousePos)) {
                scene.setCustomOrigin(mousePos);
                continue;
              }

              bool altPressed =
                  sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

              core::Figure *selFig = scene.getSelectedFigure();
              bool hitAnchor = false;
              bool hitRotationMarker = false;
              int hoveredVertex = -1;

              if (selFig) {
                float markerScale = 1.f / viewport.zoom;
                sf::Vector2f absAnchor = selFig->getAbsoluteAnchor();
                float dist = std::hypot(mousePos.x - absAnchor.x,
                                        mousePos.y - absAnchor.y);
                if (dist <= 10.f * markerScale) {
                  hitAnchor = true;
                }

                sf::FloatRect localBounds = selFig->getLocalBoundingBox();
                sf::Vector2f tl(localBounds.left, localBounds.top);
                sf::Vector2f tr(localBounds.left + localBounds.width,
                                localBounds.top);
                sf::Vector2f tc = (tl + tr) / 2.f;
                sf::Vector2f absTc = selFig->getAbsoluteVertex(tc);
                float rotRad = selFig->rotationAngle * core::math::PI / 180.f;
                sf::Vector2f rotOffset(std::sin(rotRad) * 20.f * markerScale,
                                       -std::cos(rotRad) * 20.f * markerScale);
                sf::Vector2f rotMarker = absTc + rotOffset;

                if (std::hypot(mousePos.x - rotMarker.x,
                               mousePos.y - rotMarker.y) <= 8.f * markerScale) {
                  hitRotationMarker = true;
                }

                if (isNodeEditMode) {
                  const auto &verts = selFig->getVertices();
                  for (size_t i = 0; i < verts.size(); ++i) {
                    sf::Vector2f absV = selFig->getAbsoluteVertex(verts[i]);
                    if (std::abs(mousePos.x - absV.x) <= 6.f * markerScale &&
                        std::abs(mousePos.y - absV.y) <= 6.f * markerScale) {
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
                sf::Vector2f absoluteAnchor = selFig->getAbsoluteAnchor();
                rotationStartAngle = std::atan2(mousePos.y - absoluteAnchor.y,
                                                mousePos.x - absoluteAnchor.x) *
                                     180.f / core::math::PI;
                initialRotation = selFig->rotationAngle;
              } else if (isNodeEditMode && hoveredVertex != -1) {
                draggingVertexIndex = hoveredVertex;
              } else if (hitAnchor && altPressed && selFig) {
                isDraggingAnchor = true;
                sf::Vector2f absoluteAnchor = selFig->getAbsoluteAnchor();
                dragOffset = mousePos - absoluteAnchor;
              } else {
                core::Figure *hit = scene.hitTest(mousePos);

                // Recursively check if selFig is a descendant of hit
                auto isDescendantOf = [](core::Figure* child, core::CompositeFigure* parent) -> bool {
                    for (auto& c : parent->children) {
                        if (c.figure.get() == child) return true;
                        if (auto cf = dynamic_cast<core::CompositeFigure*>(c.figure.get())) {
                            for (auto& cc : cf->children) {
                                if (cc.figure.get() == child) return true;
                            }
                        }
                    }
                    return false;
                };

                bool selFigIsChildOfHit = false;
                core::CompositeFigure* hitAsComp = nullptr;
                if (hit) hitAsComp = dynamic_cast<core::CompositeFigure*>(hit);
                if (hitAsComp && selFig && selFig != hit) {
                    selFigIsChildOfHit = isDescendantOf(selFig, hitAsComp);
                }

                if (doubleClicked && hit) {
                    if (selFigIsChildOfHit) {
                        // Already inside a group — second double-click enters node edit mode on the subfigure
                        isDrilledIntoGroup = false;
                        isNodeEditMode = true;
                    } else if (hitAsComp && !hitAsComp->children.empty()) {
                        // First double-click on a group: drill down to select a child
                        core::Figure* childHit = hitAsComp->hitTestChild(mousePos);
                        if (childHit) {
                            scene.setSelectedFigure(childHit);
                            isDrilledIntoGroup = true;
                            isNodeEditMode = false;
                        } else {
                            scene.setSelectedFigure(hit);
                            isDrilledIntoGroup = false;
                            isNodeEditMode = true;
                        }
                    } else {
                        // Double-click on a plain figure: enter node edit mode
                        isDrilledIntoGroup = false;
                        scene.setSelectedFigure(hit);
                        isNodeEditMode = true;
                    }
                } else if (hit) {
                    // Normal single click — always select the top-level figure
                    // BUT if we're drilled into a group (isDrilledIntoGroup),
                    // preserve the subfigure selection so the 2nd double-click works.
                    if (isDrilledIntoGroup && selFigIsChildOfHit) {
                        // Intermediate single-click between 2 double-clicks; keep subfigure selected.
                        // Don't activate drag immediately — wait for mouse to actually move (drag threshold).
                        isDraggingSubfigurePending = true;
                        subfigureDragStart = mousePos;
                        dragOffset = mousePos - selFig->getAbsoluteAnchor();
                    } else {
                        isDrilledIntoGroup = false;
                        scene.setSelectedFigure(hit);
                        if (selFig != hit)
                            isNodeEditMode = false;
                        isDragging = true;
                        sf::Vector2f absoluteAnchor = hit->getAbsoluteAnchor();
                        dragOffset = mousePos - absoluteAnchor;
                    }
                } else {
                    scene.setSelectedFigure(nullptr);
                    isNodeEditMode = false;
                }
              }
            } else if (currentTool == ui::Tool::Polyline) {
               bool doubleClicked = false;
               if (wasClicked && clickClock.getElapsedTime().asSeconds() < 0.3f) {
                   doubleClicked = true;
                   wasClicked = false;
               } else {
                   wasClicked = true;
                   clickClock.restart();
               }

               sf::Vector2f snapPos = mousePos;
               if (!currentPolylineVertices.empty()) {
                   if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                       sf::Vector2f lastPos = currentPolylineVertices.back();
                       sf::Vector2f delta = mousePos - lastPos;
                       if (std::abs(delta.x) > std::abs(delta.y)) snapPos = {mousePos.x, lastPos.y};
                       else snapPos = {lastPos.x, mousePos.y};
                   }
                   
                   // Auto close if clicking near first vertex
                   if (currentPolylineVertices.size() > 2) {
                       float dist = std::hypot(snapPos.x - currentPolylineVertices.front().x,
                                               snapPos.y - currentPolylineVertices.front().y);
                       if (dist < 10.f / viewport.zoom || doubleClicked) {
                           finishCurrentPolyline();
                           continue;
                       }
                   }
               }
               currentPolylineVertices.push_back(snapPos);
            } else if (currentTool == ui::Tool::CompoundSelect) {
               core::Figure* hit = scene.hitTest(mousePos);
               if (hit) {
                   auto it = std::find(compoundSelection.begin(), compoundSelection.end(), hit);
                   if (it != compoundSelection.end()) compoundSelection.erase(it);
                   else compoundSelection.push_back(hit);
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
            if (isDraggingCustomOrigin) {
              isDraggingCustomOrigin = false;
            }
            if (draggingScaleHandle != ScaleHandle::None) {
              draggingScaleHandle = ScaleHandle::None;
            }
            if (isDragging) {
              isDragging = false;
            }
            if (isDraggingSubfigure || isDraggingSubfigurePending) {
              isDraggingSubfigurePending = false;
              if (isDraggingSubfigure) {
                isDraggingSubfigure = false;
                // Snap subfigure to siblings if this is a solid group
                core::Figure* selFig2 = scene.getSelectedFigure();
                if (selFig2 && selFig2->parentFigure) {
                    if (auto* compParent = dynamic_cast<core::CompositeFigure*>(selFig2->parentFigure)) {
                        if (compParent->isSolidGroup) {
                            compParent->snapChildToSiblings(selFig2);
                        }
                    }
                }
              }
            }
            if (isDraggingAnchor) {
              isDraggingAnchor = false;
            }
            if (isRotating) {
              isRotating = false;
            }
            if (draggingVertexIndex != -1) {
              // If this is a subfigure inside a solid group, snap it to siblings
              core::Figure* selFig2 = scene.getSelectedFigure();
              if (selFig2 && selFig2->parentFigure) {
                  if (auto* compParent = dynamic_cast<core::CompositeFigure*>(selFig2->parentFigure)) {
                      if (compParent->isSolidGroup) {
                          compParent->snapChildToSiblings(selFig2);
                      }
                  }
              }
              draggingVertexIndex = -1;
            }
            if (isCreating && creatingStep == 2) {
              isCreating = false;
              creatingStep = 0;
              sf::Vector2f mousePos = viewport.screenToWorld(
                  sf::Vector2f(event.mouseButton.x, event.mouseButton.y));

              float dx = mousePos.x - createStartPos.x;
              float dy = mousePos.y - createStartPos.y;
              float width = std::abs(dx);
              float height = std::abs(dy);

              // Constrain if Shift is held
              if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
                  sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                if (currentTool == ui::Tool::Triangle) {
                  // Equilateral triangle: height = width * sqrt(3)/2
                  width = std::max(width, height / (std::sqrt(3.f) / 2.f));
                  height = width * std::sqrt(3.f) / 2.f;
                  mousePos.x = createStartPos.x + ((dx >= 0) ? width  : -width);
                  mousePos.y = createStartPos.y + ((dy >= 0) ? height : -height);
                } else {
                  float maxDim = std::max(width, height);
                  width = maxDim;
                  height = maxDim;
                  mousePos.x = createStartPos.x + ((dx >= 0) ? maxDim : -maxDim);
                  mousePos.y = createStartPos.y + ((dy >= 0) ? maxDim : -maxDim);
                }
              }

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
                if (scene.customOriginActive) {
                  fig->parentOrigin = scene.customOriginPos;
                  fig->anchor = center - scene.customOriginPos;
                } else {
                  fig->anchor = center;
                  fig->parentOrigin = sf::Vector2f(0.f, 0.f);
                }
                scene.setSelectedFigure(fig.get());
                scene.addFigure(std::move(fig));
                std::cout
                    << "Figure successfully added to scene. Total figures: "
                    << scene.figureCount() << std::endl;
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

          if (isDraggingCustomOrigin) {
            scene.setCustomOrigin(mousePos);
          } else if (isPanning) {
            sf::Vector2f currentMouse(event.mouseMove.x, event.mouseMove.y);
            sf::Vector2f delta = currentMouse - panStartMouse;
            viewport.worldOrigin = panStartOrigin + delta;
          } else if (draggingScaleHandle != ScaleHandle::None &&
                     scene.getSelectedFigure()) {
            core::Figure *selFig = scene.getSelectedFigure();
            sf::Vector2f delta = mousePos - scaleStartMouse;
            float rad = -selFig->rotationAngle * core::math::PI / 180.f;
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

            float rad2 = selFig->rotationAngle * core::math::PI / 180.f;
            float anchor_dx =
                delta_V.x * std::cos(rad2) - delta_V.y * std::sin(rad2);
            float anchor_dy =
                delta_V.x * std::sin(rad2) + delta_V.y * std::cos(rad2);

            selFig->anchor =
                scaleStartAnchor + sf::Vector2f(anchor_dx, anchor_dy);
          } else if (isRotating && scene.getSelectedFigure()) {
            sf::Vector2f absoluteAnchor =
                scene.getSelectedFigure()->getAbsoluteAnchor();
            float currentAngle = std::atan2(mousePos.y - absoluteAnchor.y,
                                            mousePos.x - absoluteAnchor.x) *
                                 180.f / core::math::PI;
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

            sf::Vector2f absoluteAnchor = scene.getSelectedFigure()->getAbsoluteAnchor();
            sf::Vector2f deltaAbs = mousePos - absoluteAnchor;
            float absRot = scene.getSelectedFigure()->getAbsoluteRotation();
            float invRad = -absRot * core::math::PI / 180.f;
            float rx =
                deltaAbs.x * std::cos(invRad) - deltaAbs.y * std::sin(invRad);
            float ry =
                deltaAbs.x * std::sin(invRad) + deltaAbs.y * std::cos(invRad);

            sf::Vector2f absScale = scene.getSelectedFigure()->getAbsoluteScale();
            float scaledX = rx / absScale.x;
            float scaledY = ry / absScale.y;
            verts[draggingVertexIndex] = sf::Vector2f(scaledX, scaledY);
          } else if (isDraggingAnchor && scene.getSelectedFigure()) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            sf::Vector2f newAbsoluteAnchor = mousePos - dragOffset;
            core::Figure* fig = scene.getSelectedFigure();

            sf::Vector2f newLocalAnchor;
            if (fig->parentFigure) {
              // Convert absolute position to parent-local coordinates
              sf::Vector2f parentAbsAnchor = fig->parentFigure->getAbsoluteAnchor();
              sf::Vector2f delta = newAbsoluteAnchor - parentAbsAnchor;
              float parentAbsRot = fig->parentFigure->getAbsoluteRotation();
              sf::Vector2f unrotated = core::math::rotate(delta, -parentAbsRot * core::math::DEG_TO_RAD);
              sf::Vector2f parentAbsScale = fig->parentFigure->getAbsoluteScale();
              newLocalAnchor = {unrotated.x / parentAbsScale.x, unrotated.y / parentAbsScale.y};
            } else {
              newLocalAnchor = newAbsoluteAnchor - fig->parentOrigin;
            }

            if (propertiesPanel.m_lockAnchor) {
              fig->anchor = newLocalAnchor;
            } else {
              fig->setAnchorKeepAbsolute(newLocalAnchor);
            }
          } else if ((isDraggingSubfigure || isDraggingSubfigurePending) && scene.getSelectedFigure()) {
            // Move subfigure in group-local coordinate space
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            // Promote pending to active only after mouse moves > 5px (drag threshold)
            if (isDraggingSubfigurePending && !isDraggingSubfigure) {
                float dx = mousePos.x - subfigureDragStart.x;
                float dy = mousePos.y - subfigureDragStart.y;
                if (std::hypot(dx, dy) > 5.f) {
                    isDraggingSubfigurePending = false;
                    isDraggingSubfigure = true;
                } else {
                    // Not yet dragging — skip movement
                    // fall through to isDragging check below
                    goto skip_subfig_drag;
                }
            }
            if (isDraggingSubfigure) {
                core::Figure* sub = scene.getSelectedFigure();
                sf::Vector2f targetWorld = mousePos - dragOffset;
                if (sub->parentFigure) {
                    sf::Vector2f parentAbs = sub->parentFigure->getAbsoluteAnchor();
                    sf::Vector2f delta = targetWorld - parentAbs;
                    float parentAbsRot = sub->parentFigure->getAbsoluteRotation();
                    sf::Vector2f unrotated = core::math::rotate(delta, -parentAbsRot * core::math::DEG_TO_RAD);
                    sf::Vector2f parentAbsScale = sub->parentFigure->getAbsoluteScale();
                    sub->anchor = {unrotated.x / parentAbsScale.x, unrotated.y / parentAbsScale.y};
                } else {
                    sub->anchor = targetWorld - sub->parentOrigin;
                }
            }
            skip_subfig_drag:;
          } else if (isDragging && scene.getSelectedFigure()) {
            sf::Vector2f mousePos = viewport.screenToWorld(
                sf::Vector2f(event.mouseMove.x, event.mouseMove.y));
            sf::Vector2f newAbsoluteAnchor = mousePos - dragOffset;
            scene.getSelectedFigure()->anchor =
                newAbsoluteAnchor - scene.getSelectedFigure()->parentOrigin;
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
      float markerScale = 1.f / viewport.zoom;
      for (int i = 0; i < 8; ++i) {
        sf::Vector2f absH = selFig->getAbsoluteVertex(handles[i]);
        if (std::hypot(mousePos.x - absH.x, mousePos.y - absH.y) <=
            8.f * markerScale) {
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

      if (isDraggingCustomOrigin || isPanning ||
          (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) &&
           currentTool == ui::Tool::Select)) {
        window.setMouseCursor(cursorHand);
      } else if (scene.customOriginActive &&
                 std::hypot(
                     sf::Mouse::getPosition(window).x -
                         viewport.worldToScreen(scene.customOriginPos).x,
                     sf::Mouse::getPosition(window).y -
                         viewport.worldToScreen(scene.customOriginPos).y) <=
                     15.f) {
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
    toolbar.render(currentTool, scene, selectedCustomToolId);
    bool fitRequested = propertiesPanel.render(scene, viewport, compoundSelection, userRegistry, toolbar);
    layerPanel.render(scene);
    createModal.render(scene, toolbar.customTools, userRegistry);

    if (toolbar.showCustomFigManager) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(600, 800));
        ImGui::Begin("Manage Custom Figures", &toolbar.showCustomFigManager);
        for (size_t i = 0; i < userRegistry.size(); ++i) {
            ImGui::PushID(i);
            char nameBuf[256];
            std::string currentName = userRegistry[i]->typeName() == "polyline" ? ((core::PolylineFigure*)userRegistry[i].get())->figureName : 
                                     (userRegistry[i]->typeName() == "composite" ? ((core::CompositeFigure*)userRegistry[i].get())->figureName : "Custom");
            strncpy(nameBuf, currentName.c_str(), sizeof(nameBuf) - 1);
            nameBuf[sizeof(nameBuf)-1] = '\0';
            
            ImGui::SetNextItemWidth(-100);
            if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
               if (userRegistry[i]->typeName() == "polyline") ((core::PolylineFigure*)userRegistry[i].get())->figureName = nameBuf;
               else if (userRegistry[i]->typeName() == "composite") ((core::CompositeFigure*)userRegistry[i].get())->figureName = nameBuf;
               if (i < toolbar.customTools.size()) {
                   toolbar.customTools[i].name = nameBuf;
               }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete", ImVec2(80, 0))) {
                userRegistry.erase(userRegistry.begin() + i);
                if (i < toolbar.customTools.size()) {
                    toolbar.customTools.erase(toolbar.customTools.begin() + i);
                }
                if (selectedCustomToolId == i) {
                    currentTool = ui::Tool::Select;
                    selectedCustomToolId = -1;
                } else if (selectedCustomToolId > (int)i) {
                    selectedCustomToolId--;
                }
                for (size_t j = i; j < toolbar.customTools.size(); ++j) {
                    toolbar.customTools[j].customId = j;
                }
                --i;
            }
            ImGui::PopID();
        }
        if (userRegistry.empty()) {
            ImGui::TextDisabled("No custom figures saved.");
        }
        ImGui::End();
    }

    if (currentTool != ui::Tool::CompoundSelect) {
        compoundSelection.clear();
    }

    if (currentTool != ui::Tool::Polyline) {
        currentPolylineVertices.clear();
    }

    if (currentTool == ui::Tool::Polyline) {
        ImGui::SetNextWindowPos(ImVec2(window.getSize().x - 320, 60), ImGuiCond_FirstUseEver);
        ImGui::Begin("Polyline Input", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Mode: click points or type values");
        if (scene.customOriginActive) {
            ImGui::TextDisabled("Custom origin: (%.1f, %.1f)", scene.customOriginPos.x, scene.customOriginPos.y);
        }
        ImGui::Separator();
        ImGui::Text("Vertices: %zu", currentPolylineVertices.size());

        if (currentPolylineVertices.empty()) {
            static bool polylineStartRelativeToCustomOrigin = true;
            if (!scene.customOriginActive) polylineStartRelativeToCustomOrigin = false;
            if (scene.customOriginActive) {
                ImGui::Checkbox("Start input relative to custom origin", &polylineStartRelativeToCustomOrigin);
            }

            ImGui::InputFloat2(scene.customOriginActive && polylineStartRelativeToCustomOrigin ? "Start dX,dY" : "Start X,Y",
                               polylineStartInput);
            if (ImGui::Button("Set start")) {
                sf::Vector2f start(polylineStartInput[0], polylineStartInput[1]);
                if (scene.customOriginActive && polylineStartRelativeToCustomOrigin) {
                    start += scene.customOriginPos;
                }
                currentPolylineVertices.push_back(start);
            }
            ImGui::TextDisabled("Or left-click to place start point.");
        } else {
            sf::Vector2f last = currentPolylineVertices.back();
            ImGui::Text("Last point: (%.1f, %.1f)", last.x, last.y);
            ImGui::InputFloat("Length", &polylineSegmentLengthInput, 1.f, 10.f, "%.3f");
            ImGui::InputFloat("Angle (deg)", &polylineSegmentAngleInputDeg, 1.f, 15.f, "%.3f");
            if (ImGui::Button("Add segment")) {
                if (polylineSegmentLengthInput > 0.f) {
                    float angleRad = polylineSegmentAngleInputDeg * core::math::PI / 180.f;
                    sf::Vector2f nextPoint(
                        last.x + polylineSegmentLengthInput * std::cos(angleRad),
                        last.y + polylineSegmentLengthInput * std::sin(angleRad));
                    currentPolylineVertices.push_back(nextPoint);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Undo point") && !currentPolylineVertices.empty()) {
                currentPolylineVertices.pop_back();
            }
        }

        bool canFinishPolyline = currentPolylineVertices.size() > 1;
        if (!canFinishPolyline) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Finish")) {
            finishCurrentPolyline();
        }
        if (!canFinishPolyline) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            currentPolylineVertices.clear();
            currentTool = ui::Tool::Select;
        }
        ImGui::End();
    }

    if (fitRequested && scene.figureCount() > 0) {
      sf::FloatRect first = scene.getFigure(0)->getBoundingBox();
      float minX = first.left, minY = first.top,
            maxX = first.left + first.width, maxY = first.top + first.height;
      for (int i = 1; i < scene.figureCount(); ++i) {
        sf::FloatRect b = scene.getFigure(i)->getBoundingBox();
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

    char statusText[256];
    snprintf(statusText, sizeof(statusText),
             "Zoom: %.0f%%   |   Cursor: (%.1f, %.1f)", viewport.zoom * 100.f,
             mPos.x, mPos.y);

    float textWidth = ImGui::CalcTextSize(statusText).x;
    float windowWidth = ImGui::GetWindowSize().x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::TextUnformatted(statusText);
    ImGui::PopStyleColor();

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

    auto drawOrigins = [&]() {
      // Origin Axes
      if (showOriginAxes) {
        sf::Vector2f activeOrigin = scene.customOriginActive
                                        ? scene.customOriginPos
                                        : sf::Vector2f(0.f, 0.f);

        sf::VertexArray originAxes(sf::Lines, 4);
        sf::Color axisColor(100, 100, 100, 150);
        sf::Vector2f boundsMin = viewport.screenToWorld(sf::Vector2f(0.f, 0.f));
        sf::Vector2f boundsMax = viewport.screenToWorld(
            sf::Vector2f(window.getSize().x, window.getSize().y));

        originAxes[0] =
            sf::Vertex(sf::Vector2f(boundsMin.x, activeOrigin.y), axisColor);
        originAxes[1] =
            sf::Vertex(sf::Vector2f(boundsMax.x, activeOrigin.y), axisColor);
        originAxes[2] =
            sf::Vertex(sf::Vector2f(activeOrigin.x, boundsMin.y), axisColor);
        originAxes[3] =
            sf::Vertex(sf::Vector2f(activeOrigin.x, boundsMax.y), axisColor);
        window.draw(originAxes);
      }

      // Always draw Global gray Marker at (0,0) World
      {
        float markerScale = 1.f / viewport.zoom;
        sf::Color crossCol(80, 80, 80);

        float length = 24.f * markerScale;
        float thickness = 2.f * markerScale;

        sf::RectangleShape hLine(sf::Vector2f(length, thickness));
        hLine.setOrigin(length / 2.f, thickness / 2.f);
        hLine.setPosition(0.f, 0.f);
        hLine.setFillColor(crossCol);

        sf::RectangleShape vLine(sf::Vector2f(thickness, length));
        vLine.setOrigin(thickness / 2.f, length / 2.f);
        vLine.setPosition(0.f, 0.f);
        vLine.setFillColor(crossCol);

        window.draw(hLine);
        window.draw(vLine);
      }

      // Draw Custom Movable Origin Marker at scene.customOriginPos
      if (scene.customOriginActive) {
        float markerScale = 1.f / viewport.zoom;
        sf::Vector2f cPos = scene.customOriginPos;

        sf::CircleShape circ(6.f * markerScale);
        circ.setOrigin(6.f * markerScale, 6.f * markerScale);
        circ.setPosition(cPos);
        circ.setFillColor(sf::Color::Transparent);
        circ.setOutlineColor(sf::Color(255, 50, 50));
        circ.setOutlineThickness(1.5f * markerScale);
        window.draw(circ);

        sf::VertexArray cross(sf::Lines, 4);
        sf::Color crossCol(255, 50, 50);
        cross[0] = sf::Vertex(sf::Vector2f(cPos.x - 12.f * markerScale, cPos.y),
                              crossCol);
        cross[1] = sf::Vertex(sf::Vector2f(cPos.x + 12.f * markerScale, cPos.y),
                              crossCol);
        cross[2] = sf::Vertex(sf::Vector2f(cPos.x, cPos.y - 12.f * markerScale),
                              crossCol);
        cross[3] = sf::Vertex(sf::Vector2f(cPos.x, cPos.y + 12.f * markerScale),
                              crossCol);
        window.draw(cross);
      }
    };

    if (!propertiesPanel.m_drawOriginsOverFigures) {
      drawOrigins();
    }

    scene.drawAll(window, 1.f / viewport.zoom);

    if (currentTool == ui::Tool::CompoundSelect) {
        for (auto fig : compoundSelection) {
            sf::FloatRect b = fig->getBoundingBox();
            sf::RectangleShape rect({b.width, b.height});
            rect.setPosition(b.left, b.top);
            rect.setFillColor(sf::Color(0, 255, 0, 50));
            rect.setOutlineColor(sf::Color::Green);
            rect.setOutlineThickness(1.f / viewport.zoom);
            window.draw(rect);
        }
    }
    
    if (currentTool == ui::Tool::CompoundSelect && !compoundSelection.empty()) {
        ImGui::SetNextWindowPos(ImVec2(window.getSize().x - 250, 60), ImGuiCond_FirstUseEver);
        ImGui::Begin("Compound Merge", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Selected: %zu figures", compoundSelection.size());
        static char compoundName[128] = "CustomGroup";
        ImGui::InputText("Name", compoundName, sizeof(compoundName));
        if (ImGui::Button("Merge into Compound")) {
            auto comp = core::CompositeFigure::mergeFromScene(compoundSelection, scene, compoundName);
            if (comp) {
                scene.addFigure(std::move(comp));
                compoundSelection.clear();
                currentTool = ui::Tool::Select;
                
                // Add to registry for Custom tool
                ui::Toolbar::CustomTool ct;
                ct.name = compoundName;
                ct.customId = userRegistry.size();
                userRegistry.push_back(scene.getFigure(scene.figureCount() - 1)->clone());
                toolbar.customTools.push_back(ct);
            }
        }
        ImGui::End();
    }
    
    if (currentTool == ui::Tool::Polyline && !currentPolylineVertices.empty()) {
        sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));
        sf::Vector2f snapPos = mousePos;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
            sf::Vector2f lastPos = currentPolylineVertices.back();
            sf::Vector2f delta = mousePos - lastPos;
            if (std::abs(delta.x) > std::abs(delta.y)) snapPos = {mousePos.x, lastPos.y};
            else snapPos = {lastPos.x, mousePos.y};
        }
        
        std::vector<sf::Vector2f> verts = currentPolylineVertices;
        verts.push_back(snapPos);
        sf::VertexArray lines(sf::LineStrip, verts.size());
        for (size_t i = 0; i < verts.size(); ++i) {
            lines[i] = sf::Vertex(verts[i], sf::Color::Black);
        }
        window.draw(lines);
    }

    if (propertiesPanel.m_drawOriginsOverFigures) {
      drawOrigins();
    }

    // Draw Anchor Marker if figure is selected
    if (scene.getSelectedFigure()) {
      float markerScale = 1.f / viewport.zoom;
      drawAnchorMarker(window,
                       scene.getSelectedFigure()->getAbsoluteAnchor(),
                       markerScale);

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
          sf::RectangleShape handle(
              sf::Vector2f(8.f * markerScale, 8.f * markerScale));
          handle.setOrigin(4.f * markerScale, 4.f * markerScale);
          handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(h));
          handle.setRotation(scene.getSelectedFigure()->rotationAngle);
          handle.setFillColor(sf::Color::White);
          handle.setOutlineColor(sf::Color(0, 120, 215));
          handle.setOutlineThickness(1.5f * markerScale);
          window.draw(handle);
        }
      }

      sf::Vector2f absTc = scene.getSelectedFigure()->getAbsoluteVertex(tc);
      float rotRad = scene.getSelectedFigure()->rotationAngle * core::math::PI / 180.f;
      sf::Vector2f rotOffset(std::sin(rotRad) * 20.f * markerScale,
                             -std::cos(rotRad) * 20.f * markerScale);
      sf::Vector2f rotPos = absTc + rotOffset;

      sf::CircleShape rotMarker(5.f * markerScale);
      rotMarker.setOrigin(5.f * markerScale, 5.f * markerScale);
      rotMarker.setPosition(rotPos);
      rotMarker.setFillColor(sf::Color::White);
      rotMarker.setOutlineColor(sf::Color(0, 120, 215));
      rotMarker.setOutlineThickness(1.5f * markerScale);
      window.draw(rotMarker);

      sf::VertexArray rotLine(sf::Lines, 2);
      rotLine[0] = sf::Vertex(absTc, sf::Color(0, 120, 215));
      rotLine[1] = sf::Vertex(rotPos, sf::Color(0, 120, 215));
      window.draw(rotLine);

      if (isNodeEditMode) {
        const auto &verts = scene.getSelectedFigure()->getVertices();
        for (const auto &v : verts) {
          sf::RectangleShape handle(
              sf::Vector2f(8.f * markerScale, 8.f * markerScale));
          handle.setOrigin(4.f * markerScale, 4.f * markerScale);
          handle.setPosition(scene.getSelectedFigure()->getAbsoluteVertex(v));
          handle.setFillColor(sf::Color::White);
          handle.setOutlineColor(sf::Color(0, 120, 215));
          handle.setOutlineThickness(1.5f * markerScale);
          window.draw(handle);
        }
      }
    }

    // Draw preview box if creating
    if (isCreating && creatingStep == 1 && currentTool != ui::Tool::Select) {
      sf::Vector2f mousePos = viewport.screenToWorld(sf::Vector2f(
          sf::Mouse::getPosition(window).x, sf::Mouse::getPosition(window).y));

      float dx = mousePos.x - createStartPos.x;
      float dy = mousePos.y - createStartPos.y;
      float width = std::abs(dx);
      float height = std::abs(dy);

      // Constrain preview if Shift is held
      if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ||
          sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        if (currentTool == ui::Tool::Triangle) {
          width = std::max(width, height / (std::sqrt(3.f) / 2.f));
          height = width * std::sqrt(3.f) / 2.f;
          mousePos.x = createStartPos.x + ((dx >= 0) ? width  : -width);
          mousePos.y = createStartPos.y + ((dy >= 0) ? height : -height);
        } else {
          float maxDim = std::max(width, height);
          width = maxDim;
          height = maxDim;
          mousePos.x = createStartPos.x + ((dx >= 0) ? maxDim : -maxDim);
          mousePos.y = createStartPos.y + ((dy >= 0) ? maxDim : -maxDim);
        }
      }

      sf::RectangleShape preview;
      preview.setPosition(std::min(createStartPos.x, mousePos.x),
                          std::min(createStartPos.y, mousePos.y));
      preview.setSize(sf::Vector2f(width, height));
      preview.setFillColor(sf::Color(150, 150, 150, 100)); // Semi-transparent
      preview.setOutlineColor(sf::Color(100, 100, 100, 200));
      preview.setOutlineThickness(1.f);
      window.draw(preview);
    }

    window.setView(oldView);
    window.resetGLStates();
    ImGui::SFML::Render(window);
    window.display();
  }

  ImGui::SFML::Shutdown();
  return 0;
}
