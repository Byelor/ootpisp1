#include "PropertiesPanel.hpp"
#include <algorithm>
#include <string>

namespace ui {

bool PropertiesPanel::render(core::Scene &scene, core::Viewport &viewport) {
  core::Figure *selectedFigure = scene.getSelectedFigure();
  bool fitRequested = false;
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 280, 0),
                          ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(280, ImGui::GetIO().DisplaySize.y),
                           ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse;

  ImGui::Begin("Properties", nullptr, flags);

  ImGui::Text("VIEWPORT PAN");
  float origin[2] = {viewport.worldOrigin.x, viewport.worldOrigin.y};
  if (ImGui::DragFloat2("##ViewportPan", origin, 1.0f)) {
    viewport.worldOrigin.x = origin[0];
    viewport.worldOrigin.y = origin[1];
  }
  if (ImGui::Button("Reset View To (0,0)")) {
    viewport.worldOrigin = sf::Vector2f(0.f, 0.f);
  }

  ImGui::Separator();
  ImGui::Text("ORIGIN POINT");

  if (!scene.customOriginActive) {
    ImGui::TextDisabled("(x) Global (0, 0)");
    ImGui::TextDisabled("( ) Custom");
  } else {
    ImGui::TextDisabled("( ) Global (0, 0)");
    ImGui::TextDisabled("(x) Custom");
    float custOrigin[2] = {scene.customOriginPos.x, scene.customOriginPos.y};
    ImGui::InputFloat2("##CustomOrigin", custOrigin, "%.1f",
                       ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Reset")) {
      scene.resetCustomOrigin();
    }
  }

  ImGui::Text("ZOOM");
  if (ImGui::Button("-")) {
    viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f,
                                 ImGui::GetIO().DisplaySize.y / 2.f),
                    1.f / 1.1f);
  }
  ImGui::SameLine();

  float zoomPct = viewport.zoom * 100.f;
  ImGui::SetNextItemWidth(80.f);
  if (ImGui::DragFloat("##Zoom", &zoomPct, 5.0f, 5.0f, 5000.0f, "%.0f%%")) {
    float oldZoom = viewport.zoom;
    viewport.zoom = std::clamp(zoomPct / 100.f, 0.05f, 50.f);
    sf::Vector2f screenCenter(ImGui::GetIO().DisplaySize.x / 2.f,
                              ImGui::GetIO().DisplaySize.y / 2.f);
    sf::Vector2f worldPoint = (screenCenter - viewport.worldOrigin) / oldZoom;
    viewport.worldOrigin = screenCenter - worldPoint * viewport.zoom;
  }

  ImGui::SameLine();
  if (ImGui::Button("+")) {
    viewport.zoomAt(sf::Vector2f(ImGui::GetIO().DisplaySize.x / 2.f,
                                 ImGui::GetIO().DisplaySize.y / 2.f),
                    1.1f);
  }

  if (ImGui::Button("Fit to Screen")) {
    fitRequested = true;
  }

  ImGui::Separator();
  ImGui::Spacing();

  if (!selectedFigure) {
    ImGui::TextDisabled("No figure selected.");
    ImGui::End();
    return fitRequested;
  }

  ImGui::Text("FIGURE PROPERTIES");
  ImGui::Separator();
  ImGui::Spacing();

  // 1. Anchor
  ImGui::Text("Anchor Point");
  float anchor[2] = {selectedFigure->anchor.x, selectedFigure->anchor.y};
  if (ImGui::DragFloat2("##Anchor", anchor, 1.0f)) {
    selectedFigure->anchor.x = anchor[0];
    selectedFigure->anchor.y = anchor[1];
  }
  ImGui::Spacing();

  // 2. Vertices
  auto &vertices = selectedFigure->getVerticesMutable();
  if (!vertices.empty()) {
    ImGui::Separator();
    ImGui::Text("Vertices (relative)");
    for (size_t i = 0; i < vertices.size(); ++i) {
      ImGui::PushID(static_cast<int>(i + 100));
      ImGui::Text("V%zu", i + 1);
      ImGui::SameLine();
      float v[2] = {vertices[i].x, vertices[i].y};
      if (ImGui::DragFloat2("##v", v, 0.5f)) {
        vertices[i].x = v[0];
        vertices[i].y = v[1];
      }
      ImGui::PopID();
    }
    ImGui::Spacing();
  }

  // 3. Fill Color
  ImGui::Separator();
  ImGui::Text("Fill Color");
  float fill[4] = {
      selectedFigure->fillColor.r / 255.f, selectedFigure->fillColor.g / 255.f,
      selectedFigure->fillColor.b / 255.f, selectedFigure->fillColor.a / 255.f};
  if (ImGui::ColorEdit4("##FillColor", fill)) {
    selectedFigure->fillColor.r = static_cast<sf::Uint8>(fill[0] * 255.f);
    selectedFigure->fillColor.g = static_cast<sf::Uint8>(fill[1] * 255.f);
    selectedFigure->fillColor.b = static_cast<sf::Uint8>(fill[2] * 255.f);
    selectedFigure->fillColor.a = static_cast<sf::Uint8>(fill[3] * 255.f);
  }
  ImGui::Spacing();

  // 4. Rotation
  ImGui::Separator();
  ImGui::Text("Rotation");
  float rotation = selectedFigure->rotationAngle;
  if (ImGui::DragFloat("Angle", &rotation, 1.0f, -360.0f, 360.0f, "%.1f deg")) {
    selectedFigure->rotationAngle = rotation;
  }
  if (ImGui::Button("Reset Rotation")) {
    selectedFigure->rotationAngle = 0.f;
  }
  ImGui::Spacing();

  // 6. Scale
  ImGui::Separator();
  ImGui::Text("Scale");
  ImGui::Checkbox("Lock Proportions", &m_lockProportions);

  float scalePct[2] = {selectedFigure->scale.x * 100.f,
                       selectedFigure->scale.y * 100.f};
  if (ImGui::DragFloat2("Scale %%##Scale", scalePct, 1.0f, 1.0f, 1000.f,
                        "%.1f%%")) {
    if (m_lockProportions) {
      if (scalePct[0] != selectedFigure->scale.x * 100.f) {
        scalePct[1] = scalePct[0];
      } else if (scalePct[1] != selectedFigure->scale.y * 100.f) {
        scalePct[0] = scalePct[1];
      }
    }
    selectedFigure->scale.x = scalePct[0] / 100.f;
    selectedFigure->scale.y = scalePct[1] / 100.f;
  }
  if (ImGui::Button("Reset Scale")) {
    selectedFigure->scale = sf::Vector2f(1.f, 1.f);
  }
  ImGui::Spacing();

  // 7. Edges
  if (!selectedFigure->edges.empty()) {
    ImGui::Separator();
    ImGui::Text("Edges");
    for (size_t i = 0; i < selectedFigure->edges.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      ImGui::Text("Edge #%zu", i + 1);

      float width = selectedFigure->edges[i].width;
      if (ImGui::DragFloat("Width", &width, 0.5f, 0.f, 100.f)) {
        selectedFigure->edges[i].width = width;
      }

      float eCol[4] = {selectedFigure->edges[i].color.r / 255.f,
                       selectedFigure->edges[i].color.g / 255.f,
                       selectedFigure->edges[i].color.b / 255.f,
                       selectedFigure->edges[i].color.a / 255.f};
      if (ImGui::ColorEdit4("Color", eCol)) {
        selectedFigure->edges[i].color.r =
            static_cast<sf::Uint8>(eCol[0] * 255.f);
        selectedFigure->edges[i].color.g =
            static_cast<sf::Uint8>(eCol[1] * 255.f);
        selectedFigure->edges[i].color.b =
            static_cast<sf::Uint8>(eCol[2] * 255.f);
        selectedFigure->edges[i].color.a =
            static_cast<sf::Uint8>(eCol[3] * 255.f);
      }
      ImGui::PopID();
      ImGui::Spacing();
    }
  }

  // 5. Bounding Box (Read only)
  ImGui::Separator();
  ImGui::Text("Bounding Box");
  sf::FloatRect bounds = selectedFigure->getBoundingBox();
  ImGui::TextDisabled("TL: %.1f, %.1f", bounds.left, bounds.top);
  ImGui::TextDisabled("BR: %.1f, %.1f", bounds.left + bounds.width,
                      bounds.top + bounds.height);

  ImGui::End();
  return fitRequested;
}

} // namespace ui
