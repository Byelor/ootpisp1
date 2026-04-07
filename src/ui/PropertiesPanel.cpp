#include "PropertiesPanel.hpp"
#include "core/Scene.hpp"
#include "core/CompositeFigure.hpp"
#include "core/PolylineFigure.hpp"
#include "core/Figures.hpp"
#include "core/MathUtils.hpp"
#include <algorithm>
#include <imgui.h>
#include <cstdio>
#include <string>

namespace ui {

bool PropertiesPanel::render(core::Scene &scene, core::Viewport &viewport, std::vector<core::Figure*>& compoundSelection, std::vector<std::unique_ptr<core::Figure>>& userRegistry, Toolbar& toolbar) {
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
  ImGui::Checkbox("Draw Over Figures", &m_drawOriginsOverFigures);

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

  char header[64];
  std::snprintf(header, sizeof(header), "FIGURE PROPERTIES [ID: %llu]", static_cast<unsigned long long>(selectedFigure->id));
  ImGui::Text("%s", header);
  ImGui::Separator();
  ImGui::Spacing();

  // 1. Anchor
  ImGui::Text("Anchor Point");
  ImGui::SameLine();
  ImGui::Checkbox("Lock to Figure", &m_lockAnchor);
  float anchor[2] = {selectedFigure->anchor.x, selectedFigure->anchor.y};
  if (ImGui::DragFloat2("##Anchor", anchor, 1.0f)) {
    if (m_lockAnchor) {
      selectedFigure->anchor = sf::Vector2f(anchor[0], anchor[1]);
    } else {
      selectedFigure->setAnchorKeepAbsolute(sf::Vector2f(anchor[0], anchor[1]));
    }
  }
  if (ImGui::Button("Reset Anchor")) {
    selectedFigure->resetAnchor();
  }
  ImGui::Spacing();

  // 2. Vertices (Global offset)
  auto &vertices = selectedFigure->getVerticesMutable();
  if (!vertices.empty() && !selectedFigure->hasUniformEdge()) {
    ImGui::Separator();
    ImGui::Text("Move by Vertex Coordinate");
    for (size_t i = 0; i < vertices.size(); ++i) {
      ImGui::PushID(static_cast<int>(i + 100));
      ImGui::Text("V%zu", i + 1);
      ImGui::SameLine();
      sf::Vector2f absV = selectedFigure->getAbsoluteVertex(vertices[i]);
      sf::Vector2f displayV = absV - selectedFigure->parentOrigin;
      float v[2] = {displayV.x, displayV.y};
      if (ImGui::DragFloat2("##v", v, 0.5f)) {
        sf::Vector2f newAbs(v[0], v[1]);
        newAbs += selectedFigure->parentOrigin;
        sf::Vector2f delta = newAbs - absV;
        selectedFigure->move(delta);
      }
      ImGui::PopID();
    }
    ImGui::Spacing();
  }

  // 3. Fill Color
  ImGui::Separator();
  ImGui::Text("Appearance");
  float col[4] = {selectedFigure->fillColor.r / 255.f,
                  selectedFigure->fillColor.g / 255.f,
                  selectedFigure->fillColor.b / 255.f,
                  selectedFigure->fillColor.a / 255.f};
  if (ImGui::ColorEdit4("Fill Color", col)) {
    sf::Color newColor(static_cast<sf::Uint8>(col[0] * 255.f),
                       static_cast<sf::Uint8>(col[1] * 255.f),
                       static_cast<sf::Uint8>(col[2] * 255.f),
                       static_cast<sf::Uint8>(col[3] * 255.f));
    
    auto applyColor = [](core::Figure* f, sf::Color c, auto& applyRef) -> void {
        f->fillColor = c;
        if (auto cf = dynamic_cast<core::CompositeFigure*>(f)) {
            for (auto& child : cf->children) {
                applyRef(child.figure.get(), c, applyRef);
            }
        }
    };
    applyColor(selectedFigure, newColor, applyColor);
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
  if (ImGui::DragFloat2("Scale %%##Scale", scalePct, 1.0f, 1.0f, 10000.f,
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
  ImGui::SameLine();
  if (ImGui::Button("Apply Scale")) {
    selectedFigure->applyScale();
  }
  ImGui::Spacing();

  // 7. Edges & Side Lengths
  bool hasLengths = selectedFigure->hasSideLengths();
  if (!selectedFigure->edges.empty() || hasLengths) {
    ImGui::Separator();
    if (ImGui::TreeNodeEx("Edges & Sides", ImGuiTreeNodeFlags_DefaultOpen)) {

      if (selectedFigure->hasUniformEdge()) {
        ImGui::PushID("UniformEdge");
        float width = selectedFigure->edges.empty()
                          ? 1.f
                          : selectedFigure->edges[0].width;
        if (ImGui::DragFloat("Width", &width, 0.5f, 0.f, 100.f)) {
          for (auto &edge : selectedFigure->edges) {
            edge.width = width;
          }
        }

        if (!selectedFigure->edges.empty()) {
          float eCol[4] = {selectedFigure->edges[0].color.r / 255.f,
                           selectedFigure->edges[0].color.g / 255.f,
                           selectedFigure->edges[0].color.b / 255.f,
                           selectedFigure->edges[0].color.a / 255.f};
          if (ImGui::ColorEdit4("Color", eCol)) {
            for (auto &edge : selectedFigure->edges) {
              edge.color.r = static_cast<sf::Uint8>(eCol[0] * 255.f);
              edge.color.g = static_cast<sf::Uint8>(eCol[1] * 255.f);
              edge.color.b = static_cast<sf::Uint8>(eCol[2] * 255.f);
              edge.color.a = static_cast<sf::Uint8>(eCol[3] * 255.f);
            }
          }
        }
        ImGui::PopID();
        ImGui::Spacing();
      } else {
        auto actualLengths = hasLengths ? selectedFigure->getSideLengths()
                                        : std::vector<float>();
        auto &lockedSides = selectedFigure->lockedSides;
        auto &lockedLengths = selectedFigure->lockedLengths;

        if (lockedSides.size() != selectedFigure->edges.size()) {
          lockedSides.resize(selectedFigure->edges.size(), false);
          lockedLengths.resize(selectedFigure->edges.size(), 1.0f);
          for (size_t i = 0; i < actualLengths.size(); ++i) {
            if (i < lockedLengths.size())
              lockedLengths[i] = actualLengths[i];
          }
        }

        std::vector<float> displayLengths = actualLengths;
        for (size_t i = 0; i < displayLengths.size(); ++i) {
          if (lockedSides[i]) {
            displayLengths[i] = lockedLengths[i];
          }
        }

        bool anyLengthChangedManually = false;

        for (size_t i = 0; i < selectedFigure->edges.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));

          if (auto circ = dynamic_cast<core::Circle*>(selectedFigure)) {
            // Circle has special radius properties instead of side lengths
            ImGui::Text("Circle Setup");
            float rx = circ->getRadiusX();
            float ry = circ->getRadiusY();
            bool rChanged = false;
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputFloat("Radius X", &rx, 1.f, 10.f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue)) rChanged = true;
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputFloat("Radius Y", &ry, 1.f, 10.f, "%.1f", ImGuiInputTextFlags_EnterReturnsTrue)) rChanged = true;
            if (rChanged) {
               if (rx < 1.f) rx = 1.f;
               if (ry < 1.f) ry = 1.f;
               circ->setRadius(rx, ry);
            }
          } else if (hasLengths && i < displayLengths.size()) {
            bool isLocked = lockedSides[i];
            if (ImGui::Checkbox("##lock", &isLocked)) {
              lockedSides[i] = isLocked;
              if (isLocked) {
                lockedLengths[i] = displayLengths[i];
              }
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Lock Side Length");
            }
            ImGui::SameLine();
          }

          if (!dynamic_cast<core::Circle*>(selectedFigure)) {
              ImGui::Text("%s", selectedFigure->getSideName(static_cast<int>(i)));
          }

          if (hasLengths && i < displayLengths.size()) {
            ImGui::SetNextItemWidth(-1.f);
            if (ImGui::InputFloat("Length", &displayLengths[i], 1.f, 10.f,
                                  "%.1f",
                                  ImGuiInputTextFlags_EnterReturnsTrue)) {
              if (displayLengths[i] < 1.f)
                displayLengths[i] = 1.f;

              if (lockedSides[i]) {
                lockedLengths[i] = displayLengths[i];
              }
              anyLengthChangedManually = true;
            }
          }

          float width = selectedFigure->edges[i].width;
          if (ImGui::DragFloat("Width", &width, 0.5f, 0.f, 100.f)) {
            selectedFigure->edges[i].width = width;
          }

          (void)0; // vertex angles shown in dedicated section below

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

        if (anyLengthChangedManually) {
          selectedFigure->applyScale();
          bool anyLockedNow = false;
          for (bool l : selectedFigure->lockedSides) {
            if (l)
              anyLockedNow = true;
          }
          if (anyLockedNow) {
            selectedFigure->applyGenericSideLengths(displayLengths);
          } else {
            selectedFigure->setSideLengths(displayLengths);
          }
        }
      }

      ImGui::TreePop();
    }
    ImGui::Spacing();
  }

  // 6b. Vertex Angles (only for PolylineFigure)
  if (auto* pf = dynamic_cast<core::PolylineFigure*>(selectedFigure)) {
      int vn = static_cast<int>(pf->getVertices().size());
      if (!dynamic_cast<core::Circle*>(selectedFigure) && vn >= 3) {
          ImGui::Separator();
          if (ImGui::TreeNodeEx("Vertex Angles", ImGuiTreeNodeFlags_DefaultOpen)) {
              auto& lockedAng  = pf->lockedAngles;
              auto& lockedAngV = pf->lockedAngleValues;
              // Initialize or reset if sizes changed or values are stale (> 360 means old formula)
              bool needsReset = ((int)lockedAng.size() != vn);
              if (!needsReset) {
                  for (float v : lockedAngV) {
                      if (v < 0.f || v > 360.f) { needsReset = true; break; }
                  }
              }
              if (needsReset) {
                  lockedAng.assign(vn, false);
                  lockedAngV.resize(vn, 0.f);
                  for (int i = 0; i < vn; ++i)
                      lockedAngV[i] = pf->getVertexAngle(i);
              }
              for (int i = 0; i < vn; ++i) {
                  ImGui::PushID(static_cast<int>(5000 + i));
                  bool isLocked = lockedAng[i];
                  if (ImGui::Checkbox("##alock", &isLocked)) {
                      lockedAng[i] = isLocked;
                      if (isLocked) lockedAngV[i] = pf->getVertexAngle(i);
                  }
                  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Lock vertex angle");
                  ImGui::SameLine();
                  ImGui::Text("V%d:", i);
                  ImGui::SameLine();

                  if (isLocked) {
                      ImGui::SetNextItemWidth(-1.f);
                      ImGui::InputFloat("##va", &lockedAngV[i], 0.f, 0.f, "%.1f deg");
                  } else {
                      float angle = pf->getVertexAngle(i);
                      ImGui::SetNextItemWidth(-1.f);
                      if (ImGui::DragFloat("##va", &angle, 0.5f, 1.f, 359.f, "%.1f\xc2\xb0")) {
                          pf->setVertexAngle(i, angle);
                      }
                  }
                  ImGui::PopID();
              }
              // One Apply button for all locked angles
              bool anyLocked = false;
              for (bool b : lockedAng) if (b) { anyLocked = true; break; }
              if (anyLocked) {
                  if (ImGui::Button("Apply Locked Angles", ImVec2(-1.f, 0.f))) {
                      for (int i = 0; i < vn; ++i) {
                          if (lockedAngV[i] < 1.f)   lockedAngV[i] = 1.f;
                          if (lockedAngV[i] > 359.f) lockedAngV[i] = 359.f;
                      }
                      // Run many iterations so all locked angles converge simultaneously
                      for (int iter = 0; iter < 100; ++iter) {
                          for (int i = 0; i < vn; ++i) {
                              if (!lockedAng[i]) continue;
                              pf->setVertexAngle(i, lockedAngV[i]);
                          }
                      }
                  }
              }
              ImGui::TreePop();
          }
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

  if (!compoundSelection.empty()) {
      ImGui::Separator();
      ImGui::Text("COMPOUND SELECTION (%zu)", compoundSelection.size());
      if (ImGui::Button("Group Selected", ImVec2(-1, 0))) {
          auto newCompound = std::make_unique<core::CompositeFigure>();
          newCompound->figureName = "Grouped Figure";
          
          sf::Vector2f center(0.f, 0.f);
          for (auto* fig : compoundSelection) {
              center += fig->anchor + fig->parentOrigin;
          }
          center.x /= compoundSelection.size();
          center.y /= compoundSelection.size();
          newCompound->anchor = center;

          for (auto* fig : compoundSelection) {
              sf::Vector2f offset = fig->anchor + fig->parentOrigin - center;
              float rot = fig->rotationAngle;
              newCompound->insertChild(scene.extractFigure(fig), newCompound->children.size(), offset, rot);
          }
          auto* ptr = newCompound.get();
          scene.addFigure(std::move(newCompound));
          scene.setSelectedFigure(ptr);
          compoundSelection.clear();
      }
      ImGui::Spacing();
  } else if (auto cf = dynamic_cast<core::CompositeFigure*>(selectedFigure)) {
      if (!cf->children.empty()) {
          ImGui::Separator();
          ImGui::Text("Name: %s", cf->figureName.c_str());
          ImGui::Text("Child Figures (%zu)", cf->children.size());

          // Solid Group toggle
          bool isSolid = cf->isSolidGroup;
          if (ImGui::Checkbox("Solid Group", &isSolid)) {
              cf->isSolidGroup = isSolid;
          }
          if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("Subfigures snap to each other —\ncannot be pulled apart.");
          }
          ImGui::Text("Global Outline Properties");
          static float globalOutlineWidth = 1.0f;
          if (ImGui::DragFloat("Outline Width##Global", &globalOutlineWidth, 0.5f, 0.f, 100.f)) {
              auto applyOutline = [](core::Figure* f, float w, auto& applyRef) -> void {
                  for (auto& edge : f->edges) edge.width = w;
                  if (auto cFig = dynamic_cast<core::CompositeFigure*>(f)) {
                      for (auto& child : cFig->children) applyRef(child.figure.get(), w, applyRef);
                  }
              };
              applyOutline(cf, globalOutlineWidth, applyOutline);
          }
          static float globalOutlineCol[4] = {0.f, 0.f, 0.f, 1.f};
          if (ImGui::ColorEdit4("Outline Color##Global", globalOutlineCol)) {
              sf::Color newColor(static_cast<sf::Uint8>(globalOutlineCol[0] * 255.f),
                                 static_cast<sf::Uint8>(globalOutlineCol[1] * 255.f),
                                 static_cast<sf::Uint8>(globalOutlineCol[2] * 255.f),
                                 static_cast<sf::Uint8>(globalOutlineCol[3] * 255.f));
              auto applyOutlineColor = [](core::Figure* f, sf::Color c, auto& applyRef) -> void {
                  for (auto& edge : f->edges) edge.color = c;
                  if (auto cFig = dynamic_cast<core::CompositeFigure*>(f)) {
                      for (auto& child : cFig->children) applyRef(child.figure.get(), c, applyRef);
                  }
              };
              applyOutlineColor(cf, newColor, applyOutlineColor);
          }
          ImGui::Separator();
          
          for (size_t i = 0; i < cf->children.size(); ++i) {
              ImGui::PushID(static_cast<int>(2000 + i));
              ImGui::Text("Child %zu", i);
              float off[2] = {cf->children[i].figure->anchor.x, cf->children[i].figure->anchor.y};
              if (ImGui::DragFloat2("Rel Pos", off, 1.0f)) {
                  cf->children[i].figure->anchor = {off[0], off[1]};
              }
              ImGui::DragFloat("Rel Rot", &cf->children[i].figure->rotationAngle, 1.0f);
              if (ImGui::Button("Extract")) {
                  auto extracted = cf->extractChild(cf->children[i].figure.get());
                  if (extracted) {
                      // extractChild already set anchor to the absolute world position.
                      // Accumulate parent rotation and scale so the figure looks the same.
                      float parentAbsRot = cf->getAbsoluteRotation();
                      sf::Vector2f parentAbsScale = cf->getAbsoluteScale();
                      extracted->rotationAngle += parentAbsRot;
                      extracted->scale.x *= parentAbsScale.x;
                      extracted->scale.y *= parentAbsScale.y;
                      scene.addFigure(std::move(extracted));
                  }
              }
              ImGui::SameLine();
              if (ImGui::Button("Delete")) {
                  cf->extractChild(cf->children[i].figure.get());
                  // Extracted unique_ptr drops out of scope = deleted
              }
              ImGui::PopID();
              ImGui::Spacing();
          }
      }
  }

  ImGui::End();
  return fitRequested;
}

} // namespace ui
