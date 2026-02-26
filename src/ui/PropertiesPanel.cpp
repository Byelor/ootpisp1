#include "PropertiesPanel.hpp"
#include <string>

namespace ui {

void PropertiesPanel::render(core::Figure *selectedFigure) {
  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 280, 0),
                          ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(280, ImGui::GetIO().DisplaySize.y),
                           ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse;

  ImGui::Begin("Properties", nullptr, flags);

  if (!selectedFigure) {
    ImGui::TextDisabled("No figure selected.");
    ImGui::End();
    return;
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

  // 4. Edges
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
}

} // namespace ui
