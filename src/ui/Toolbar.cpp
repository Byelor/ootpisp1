#include "Toolbar.hpp"

namespace ui {

bool Toolbar::render(Tool &currentTool) {
  bool toolChanged = false;

  // Fixed position left toolbar
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(80, ImGui::GetIO().DisplaySize.y),
                           ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("Toolbar", nullptr, flags);

  auto renderToolButton = [&](const char *label, Tool expectedTool) {
    bool isActive = (currentTool == expectedTool);
    if (isActive) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }

    if (ImGui::Button(label, ImVec2(-1, 40))) {
      if (!isActive) {
        currentTool = expectedTool;
        toolChanged = true;
      }
    }

    if (isActive) {
      ImGui::PopStyleColor();
    }
  };

  renderToolButton("Select", Tool::Select);
  ImGui::Separator();
  renderToolButton("Rect", Tool::Rectangle);
  renderToolButton("Tri", Tool::Triangle);
  renderToolButton("Hex", Tool::Hexagon);
  renderToolButton("Rhombus", Tool::Rhombus);
  renderToolButton("Trapezoid", Tool::Trapezoid);

  ImGui::End();

  return toolChanged;
}

} // namespace ui
