#include "CreateFigureModal.hpp"
#include "core/CompositeFigure.hpp"
#include "core/PolylineFigure.hpp"
#include "core/Figures.hpp"
#include <imgui.h>

namespace ui {

static const char *kFigureNames[] = {"Rectangle (4)", "Triangle (3)",
                                     "Hexagon (6)",   "Rhombus (4)",
                                     "Trapezoid (4)", "Circle"};
static int kEdgeCounts[] = {4, 3, 6, 4, 4, 1};

void CreateFigureModal::open(sf::Vector2f pos) {
  m_open = true;
  m_createPos = pos;
  resetDefaults();
}

void CreateFigureModal::resetDefaults() {
  m_figureType = 0;
  m_radiusX = 50.f;
  m_radiusY = 50.f;
  m_fillColor[0] = 0.6f;
  m_fillColor[1] = 0.6f;
  m_fillColor[2] = 0.6f;
  m_fillColor[3] = 1.0f;
  reinitEdges();
}

void CreateFigureModal::reinitEdges() {
  int n = kEdgeCounts[m_figureType];
  m_edges.resize(n);
  for (auto &e : m_edges) {
    e.length = 100.f;
    e.width = 2.0f;
    e.color[0] = 0.f;
    e.color[1] = 0.f;
    e.color[2] = 0.f;
    e.color[3] = 1.f;
  }
}

std::unique_ptr<core::Figure> CreateFigureModal::createConfiguredFigure(const std::vector<std::unique_ptr<core::Figure>>& userRegistry) {
  std::unique_ptr<core::Figure> fig;
  std::vector<float> lengths;
  for (auto &e : m_edges)
    lengths.push_back(e.length);

  if (m_figureType >= 6) {
      int customIdx = m_figureType - 6;
      if (customIdx >= 0 && customIdx < (int)userRegistry.size()) {
          fig = userRegistry[customIdx]->clone();
          if (fig->typeName() == "polyline") {
             ((core::PolylineFigure*)fig.get())->figureName = m_customName;
          } else if (fig->typeName() == "composite") {
             ((core::CompositeFigure*)fig.get())->figureName = m_customName;
          }
      }
  } else {
      switch (m_figureType) {
      case 0: { // Rectangle
        fig = std::make_unique<core::Rectangle>(lengths[0], lengths[3]);
        break;
      }
  case 1: { // Triangle — build default then adjust
    fig = std::make_unique<core::Triangle>(lengths[0], 100.f);
    break;
  }
  case 2: { // Hexagon
    float avg = 0.f;
    for (auto l : lengths)
      avg += l;
    avg /= lengths.size();
    fig = std::make_unique<core::Hexagon>(avg * 2.f, avg * 2.f);
    break;
  }
  case 3: { // Rhombus
    float avg = 0.f;
    for (auto l : lengths)
      avg += l;
    avg /= lengths.size();
    fig = std::make_unique<core::Rhombus>(avg * 2.f, avg * 2.f);
    break;
  }
  case 4: { // Trapezoid
    fig = std::make_unique<core::Trapezoid>(lengths[0], lengths[2], 100.f);
    break;
  }
  case 5: { // Circle
    fig = std::make_unique<core::Circle>(m_radiusX, m_radiusY);
    break;
  }
  }
  }

  if (!fig)
    return nullptr;

  // Apply geometry via setSideLengths
  if (fig->hasSideLengths()) {
    fig->setSideLengths(lengths);
  }

  fig->fillColor = sf::Color(static_cast<sf::Uint8>(m_fillColor[0] * 255),
                             static_cast<sf::Uint8>(m_fillColor[1] * 255),
                             static_cast<sf::Uint8>(m_fillColor[2] * 255),
                             static_cast<sf::Uint8>(m_fillColor[3] * 255));

  for (size_t i = 0; i < m_edges.size() && i < fig->edges.size(); ++i) {
    fig->edges[i].width = m_edges[i].width;
    fig->edges[i].color =
        sf::Color(static_cast<sf::Uint8>(m_edges[i].color[0] * 255),
                  static_cast<sf::Uint8>(m_edges[i].color[1] * 255),
                  static_cast<sf::Uint8>(m_edges[i].color[2] * 255),
                  static_cast<sf::Uint8>(m_edges[i].color[3] * 255));
  }
  return fig;
}

// Side names per figure type, edge index
static const char *getSideName(int figureType, int idx) {
  static const char *rect4[] = {"Top", "Right", "Bottom", "Left"};
  static const char *trap4[] = {"Top", "Right leg", "Bottom", "Left leg"};
  static const char *tri3[] = {"Bottom", "Right side", "Left side"};
  static const char *hex6[] = {"Side 1", "Side 2", "Side 3",
                               "Side 4", "Side 5", "Side 6"};
  static const char *rho4[] = {"Top-R", "Bottom-R", "Bottom-L", "Top-L"};
  switch (figureType) {
  case 0:
    return idx < 4 ? rect4[idx] : "?";
  case 1:
    return idx < 3 ? tri3[idx] : "?";
  case 2:
    return idx < 6 ? hex6[idx] : "?";
  case 3:
    return idx < 4 ? rho4[idx] : "?";
  case 4:
    return idx < 4 ? trap4[idx] : "?";
  default: {
    static char buf[32];
    snprintf(buf, sizeof(buf), "Side %d", idx + 1);
    return buf;
  }
  }
}

void CreateFigureModal::render(core::Scene &scene, const std::vector<Toolbar::CustomTool>& customTools, const std::vector<std::unique_ptr<core::Figure>>& userRegistry) {
  if (m_open)
    ImGui::OpenPopup("Create Figure##Modal");

  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                 ImGui::GetIO().DisplaySize.y * 0.5f),
                          ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSizeConstraints(ImVec2(380, 300), ImVec2(500, 700));

  if (ImGui::BeginPopupModal("Create Figure##Modal", &m_open,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    int previousType = m_figureType;
    
    std::vector<const char*> allNames;
    for (int i = 0; i < 6; ++i) allNames.push_back(kFigureNames[i]);
    for (const auto& ct : customTools) allNames.push_back(ct.name.c_str());

    if (ImGui::Combo("Figure Type", &m_figureType, allNames.data(),
                     (int)allNames.size())) {
      if (m_figureType != previousType) {
          if (m_figureType < 6) {
              reinitEdges();
          } else {
              int customIdx = m_figureType - 6;
              if (customIdx >= 0 && customIdx < (int)userRegistry.size()) {
                  const auto& regFig = userRegistry[customIdx];
                  m_edges.clear();
                  m_edges.resize(regFig->edges.size());
                  std::vector<float> lengths = regFig->getSideLengths();
                  for (size_t i = 0; i < m_edges.size(); ++i) {
                      m_edges[i].length = (i < lengths.size()) ? lengths[i] : 100.f;
                      m_edges[i].width = regFig->edges[i].width;
                      m_edges[i].color[0] = regFig->edges[i].color.r / 255.f;
                      m_edges[i].color[1] = regFig->edges[i].color.g / 255.f;
                      m_edges[i].color[2] = regFig->edges[i].color.b / 255.f;
                      m_edges[i].color[3] = regFig->edges[i].color.a / 255.f;
                  }
                  m_fillColor[0] = regFig->fillColor.r / 255.f;
                  m_fillColor[1] = regFig->fillColor.g / 255.f;
                  m_fillColor[2] = regFig->fillColor.b / 255.f;
                  m_fillColor[3] = regFig->fillColor.a / 255.f;
                  
                  if (customIdx < (int)customTools.size()) {
                      strncpy(m_customName, customTools[customIdx].name.c_str(), sizeof(m_customName) - 1);
                  }
              } else {
                  m_edges.clear();
              }
          }
      }
    }

    ImGui::Separator();
    
    if (m_figureType >= 6) {
        ImGui::Text("Custom Figure: %s", allNames[m_figureType]);
        ImGui::InputText("Instance Name", m_customName, sizeof(m_customName));
        ImGui::Separator();
    }
    
    ImGui::Text("Appearance");
    ImGui::ColorEdit4("Fill Color", m_fillColor);

        ImGui::Separator();

        if (m_figureType == 5) {
      // Circle: just radius inputs
      ImGui::Text("Geometry");
      ImGui::DragFloat("Radius X", &m_radiusX, 1.f, 5.f, 2000.f);
      ImGui::DragFloat("Radius Y", &m_radiusY, 1.f, 5.f, 2000.f);
      ImGui::Separator();
      ImGui::Text("Outline");
      ImGui::PushID(0);
      ImGui::DragFloat("Thickness", &m_edges[0].width, 0.5f, 0.f, 100.f);
      ImGui::ColorEdit4("Color##e", m_edges[0].color,
                        ImGuiColorEditFlags_NoInputs);
      ImGui::PopID();
    } else {
      // Polygon — show per-side table
      ImGui::Text("Side Lengths, Thickness & Color");
      ImGui::BeginChild("SidesScroll", ImVec2(0, 0), true,
                        ImGuiWindowFlags_HorizontalScrollbar);

      ImGui::Columns(4, "SidesTable", true);
      ImGui::SetColumnWidth(0, 90.f);
      ImGui::SetColumnWidth(1, 80.f);
      ImGui::SetColumnWidth(2, 80.f);
      ImGui::SetColumnWidth(3, 50.f);
      ImGui::Text("Side");
      ImGui::NextColumn();
      ImGui::Text("Length");
      ImGui::NextColumn();
      ImGui::Text("Thickness");
      ImGui::NextColumn();
      ImGui::Text("Color");
      ImGui::NextColumn();
      ImGui::Separator();

      for (int i = 0; i < (int)m_edges.size(); ++i) {
        ImGui::PushID(i);
        ImGui::TextUnformatted(getSideName(m_figureType, i));
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1.f);
        ImGui::DragFloat("##len", &m_edges[i].length, 1.f, 10.f, 2000.f,
                         "%.0f");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(-1.f);
        ImGui::DragFloat("##thk", &m_edges[i].width, 0.5f, 0.f, 100.f, "%.1f");
        ImGui::NextColumn();
        ImGui::ColorEdit4("##col", m_edges[i].color,
                          ImGuiColorEditFlags_NoInputs |
                              ImGuiColorEditFlags_NoLabel);
        ImGui::NextColumn();
        ImGui::PopID();
      }
      ImGui::Columns(1);
      ImGui::EndChild();
    }

    ImGui::Separator();
    if (ImGui::Button("Create", ImVec2(120, 0))) {
      auto fig = createConfiguredFigure(userRegistry);
      if (fig) {
        if (scene.customOriginActive) {
          fig->parentOrigin = scene.customOriginPos;
          fig->anchor = m_createPos - scene.customOriginPos;
        } else {
          fig->parentOrigin = sf::Vector2f(0.f, 0.f);
          fig->anchor = m_createPos;
        }
        scene.setSelectedFigure(fig.get());
        scene.addFigure(std::move(fig));
      }
      m_open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      m_open = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

} // namespace ui
