#include "Toolbar.hpp"
#include "core/Scene.hpp"
#include "core/SceneSerializer.hpp"

namespace ui {

bool Toolbar::render(Tool &currentTool, core::Scene& scene, int& selectedCustomToolId) {
  bool toolChanged = false;

  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 60),
                           ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("Toolbar", nullptr, flags);

  if (ImGui::Button("Save", ImVec2(60, 40))) ImGui::OpenPopup("Save Scene");
  ImGui::SameLine();
  if (ImGui::BeginPopupModal("Save Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      static char savePath[512] = "scene.scene";
      ImGui::InputText("path", savePath, sizeof(savePath));
      if (ImGui::Button("Save", ImVec2(120, 0))) {
          core::SceneSerializer::save(scene, savePath);
          ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
  }

  if (ImGui::Button("Load", ImVec2(60, 40))) ImGui::OpenPopup("Load Scene");
  ImGui::SameLine();
  if (ImGui::BeginPopupModal("Load Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      static char loadPath[512] = "scene.scene";
      ImGui::InputText("path", loadPath, sizeof(loadPath));
      if (ImGui::Button("Load", ImVec2(120, 0))) {
          // core::Scene doesn't have clear(), but scene array does. 
          // Wait, scene.clear() might not exist. Let's see if scene.m_figures is accessible.
          // Better: just remove all figures.
          while(scene.figureCount() > 0) {
              scene.removeFigure(scene.getFigure(0));
          }
          core::SceneSerializer::load(scene, loadPath);
          scene.setSelectedFigure(nullptr);
          ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
          ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
  }

  // Separator can be drawn vertically or omitted, we'll just omit or add spacing
  ImGui::Dummy(ImVec2(10, 0));
  ImGui::SameLine();

  auto renderToolButton = [&](const char *label, Tool expectedTool) {
    bool isActive = (currentTool == expectedTool);
    if (isActive) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }

    if (ImGui::Button(label, ImVec2(65, 40))) {
      if (!isActive) {
        currentTool = expectedTool;
        if (expectedTool != Tool::Custom) {
            selectedCustomToolId = -1;
        }
        toolChanged = true;
      }
    }

    if (isActive) {
      ImGui::PopStyleColor();
    }
  };

  renderToolButton("Select", Tool::Select);
  ImGui::SameLine();
  renderToolButton("Rect", Tool::Rectangle);
  ImGui::SameLine();
  renderToolButton("Tri", Tool::Triangle);
  ImGui::SameLine();
  renderToolButton("Hex", Tool::Hexagon);
  ImGui::SameLine();
  renderToolButton("Rhombus", Tool::Rhombus);
  ImGui::SameLine();
  renderToolButton("Trapezoid", Tool::Trapezoid);
  ImGui::SameLine();
  renderToolButton("Circle", Tool::Circle);
  ImGui::SameLine();
  renderToolButton("Polyline", Tool::Polyline);
  ImGui::SameLine();
  renderToolButton("Compound", Tool::CompoundSelect);

  if (!customTools.empty()) {
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(10, 0));
      ImGui::SameLine();
      ImGui::SetNextItemWidth(150.f);
      
      std::string preview = "Custom Figures";
      for (const auto& ct : customTools) {
          if (currentTool == Tool::Custom && selectedCustomToolId == ct.customId) {
              preview = ct.name;
              break;
          }
      }

      if (ImGui::BeginCombo("##CustomDropdown", preview.c_str())) {
          for (const auto& ct : customTools) {
              bool isSelected = (currentTool == Tool::Custom && selectedCustomToolId == ct.customId);
              if (ImGui::Selectable(ct.name.c_str(), isSelected)) {
                  currentTool = Tool::Custom;
                  selectedCustomToolId = ct.customId;
                  toolChanged = true;
              }
              if (isSelected) ImGui::SetItemDefaultFocus();
          }
          ImGui::EndCombo();
      }
  }

  ImGui::End();

  return toolChanged;
}

} // namespace ui
