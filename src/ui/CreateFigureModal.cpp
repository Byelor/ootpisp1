#include "CreateFigureModal.hpp"
#include "core/CompositeFigure.hpp"
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

std::unique_ptr<core::Figure> CreateFigureModal::createConfiguredFigure() {
  std::unique_ptr<core::Figure> fig;
  std::vector<float> lengths;
  for (auto &e : m_edges)
    lengths.push_back(e.length);

  switch (m_figureType) {
  case 0: { // Rectangle
    fig = core::CompositeFigure::createRectangle(lengths[0], lengths[3]);
    break;
  }
  case 1: { // Triangle — build default then adjust
    fig = core::CompositeFigure::createTriangle(lengths[0], 100.f);
    break;
  }
  case 2: { // Hexagon
    float avg = 0.f;
    for (auto l : lengths)
      avg += l;
    avg /= lengths.size();
    fig = core::CompositeFigure::createHexagon(avg * 2.f, avg * 2.f);
    break;
  }
  case 3: { // Rhombus
    float avg = 0.f;
    for (auto l : lengths)
      avg += l;
    avg /= lengths.size();
    fig = core::CompositeFigure::createRhombus(avg * 2.f, avg * 2.f);
    break;
  }
  case 4: { // Trapezoid
    fig = core::CompositeFigure::createTrapezoid(lengths[0], lengths[2], 100.f);
    break;
  }
  case 5: { // Circle
    fig = core::CompositeFigure::createCircle(m_radiusX, m_radiusY);
    break;
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
  default:
    return "Outline";
  }
}

void CreateFigureModal::render(core::Scene &scene) {
  if (m_open)
    ImGui::OpenPopup("Create Figure##Modal");

  ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
                                 ImGui::GetIO().DisplaySize.y * 0.5f),
                          ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSizeConstraints(ImVec2(380, 300), ImVec2(500, 700));

  if (ImGui::BeginPopupModal("Create Figure##Modal", &m_open,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    int previousType = m_figureType;
    if (ImGui::Combo("Figure Type", &m_figureType, kFigureNames,
                     IM_ARRAYSIZE(kFigureNames))) {
      if (m_figureType != previousType)
        reinitEdges();
    }

    ImGui::Separator();
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
      auto fig = createConfiguredFigure();
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
