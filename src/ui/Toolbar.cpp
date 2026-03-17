#include "Toolbar.hpp"
#include "core/Scene.hpp"
#include "core/SceneSerializer.hpp"

namespace ui {

bool Toolbar::render(Tool &currentTool, core::Scene& scene, int& selectedCustomToolId) {
  bool toolChanged = false;

  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(100, ImGui::GetIO().DisplaySize.y),
                           ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::Begin("Toolbar", nullptr, flags);

  if (ImGui::Button("Save", ImVec2(-1, 0))) ImGui::OpenPopup("Save Scene");
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

  if (ImGui::Button("Load", ImVec2(-1, 0))) ImGui::OpenPopup("Load Scene");
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

  ImGui::Separator();

  auto renderToolButton = [&](const char *label, Tool expectedTool) {
    bool isActive = (currentTool == expectedTool);
    if (isActive) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }

    if (ImGui::Button(label, ImVec2(-1, 40))) {
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
  ImGui::Separator();
  renderToolButton("Rect", Tool::Rectangle);
  renderToolButton("Tri", Tool::Triangle);
  renderToolButton("Hex", Tool::Hexagon);
  renderToolButton("Rhombus", Tool::Rhombus);
  renderToolButton("Trapezoid", Tool::Trapezoid);
  renderToolButton("Circle", Tool::Circle);
  ImGui::Separator();
  renderToolButton("Polyline", Tool::Polyline);
  renderToolButton("Cpnd Sel", Tool::CompoundSelect);

  if (!customTools.empty()) {
      ImGui::Separator();
      ImGui::Text("Custom:");
      for (const auto& ct : customTools) {
          bool isActive = (currentTool == Tool::Custom && selectedCustomToolId == ct.customId);
          if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
          if (ImGui::Button(ct.name.c_str(), ImVec2(-1, 40))) {
              if (!isActive) {
                  currentTool = Tool::Custom;
                  selectedCustomToolId = ct.customId;
                  toolChanged = true;
              }
          }
          if (isActive) ImGui::PopStyleColor();
      }
  }

  ImGui::End();

  return toolChanged;
}

} // namespace ui
