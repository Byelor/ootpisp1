#include "CreateFigureModal.hpp"
#include "core/CompositeFigure.hpp"
#include "core/PolylineFigure.hpp"
#include "core/Figures.hpp"
#include "core/MathUtils.hpp"
#include "core/SceneSerializer.hpp"
#include <filesystem>
#include <imgui.h>
#include <cmath>
#include <cstring>

namespace fs = std::filesystem;

namespace ui {

// ─── helpers ──────────────────────────────────────────────────────────────────

void CreateFigureModal::open(sf::Vector2f pos) {
  m_open = true;
  m_createPos = pos;
  drawOnCanvasRequested = false;
  m_polyStatusMsg.clear();
  resetDefaults();
}

void CreateFigureModal::resetDefaults() {
  m_figureType = 0;
  m_radiusX = 50.f;
  m_radiusY = 50.f;
  m_fillColor[0] = m_fillColor[1] = m_fillColor[2] = 0.6f;
  m_fillColor[3] = 1.f;
  reinitEdges();
}

void CreateFigureModal::reinitEdges() {
  int n = (m_figureType == 0) ? 1 : 0; // Circle=1, else 0
  m_edges.resize(n);
  for (auto& e : m_edges) {
    e.length = 100.f; e.width = 2.f;
    e.color[0] = e.color[1] = e.color[2] = 0.f; e.color[3] = 1.f;
  }
}

// ─── buildPolylineFromSegments ────────────────────────────────────────────────
std::unique_ptr<core::Figure> CreateFigureModal::buildPolylineFromSegments() {
  if (m_polySegments.empty()) return nullptr;

  // Trace vertices
  std::vector<sf::Vector2f> pts;
  sf::Vector2f cur(0.f, 0.f);
  pts.push_back(cur);
  for (auto& seg : m_polySegments) {
    float rad = seg.angleDeg * core::math::PI / 180.f;
    cur.x += seg.length * std::cos(rad);
    cur.y += seg.length * std::sin(rad);
    pts.push_back(cur);
  }

  // Center bounding-box at (0,0)
  sf::Vector2f minP = pts[0], maxP = pts[0];
  for (auto& p : pts) {
    minP.x = std::min(minP.x, p.x); minP.y = std::min(minP.y, p.y);
    maxP.x = std::max(maxP.x, p.x); maxP.y = std::max(maxP.y, p.y);
  }
  sf::Vector2f center = (minP + maxP) / 2.f;

  auto fig = std::make_unique<core::PolylineFigure>();
  fig->figureName = m_polyTemplateName;

  std::vector<sf::Vector2f> local;
  for (auto& p : pts) local.push_back(p - center);
  fig->setVertices(local);
  fig->edges.resize(local.size());

  fig->fillColor = sf::Color(
      (sf::Uint8)(m_polyFillColor[0]*255),
      (sf::Uint8)(m_polyFillColor[1]*255),
      (sf::Uint8)(m_polyFillColor[2]*255),
      (sf::Uint8)(m_polyFillColor[3]*255));
  for (auto& e : fig->edges) {
    e.width = m_polyEdgeWidth;
    e.color = sf::Color(
        (sf::Uint8)(m_polyEdgeColor[0]*255),
        (sf::Uint8)(m_polyEdgeColor[1]*255),
        (sf::Uint8)(m_polyEdgeColor[2]*255),
        (sf::Uint8)(m_polyEdgeColor[3]*255));
  }
  return fig;
}

// ─── savePolylineAsTemplate ───────────────────────────────────────────────────
void CreateFigureModal::savePolylineAsTemplate(
    std::vector<Toolbar::CustomTool>& customTools,
    std::vector<std::unique_ptr<core::Figure>>& userRegistry)
{
  auto fig = buildPolylineFromSegments();
  if (!fig) { m_polyStatusMsg = "No segments to save!"; return; }

  // Sanitise name
  std::string name = m_polyTemplateName;
  if (name.empty()) name = "my_figure";
  for (auto& c : name)
    if (c == ' ' || c == '/' || c == '\\' || c == ':') c = '_';

  fs::create_directories("figures");
  std::string path = "figures/" + name + ".fig";

  // Clone for template (anchor stays at 0)
  auto templateFig = fig->clone();
  if (!core::SceneSerializer::saveFigureTemplate(templateFig.get(), path)) {
    m_polyStatusMsg = "Error: could not write " + path;
    return;
  }

  // Upsert: overwrite if a template with this name already exists
  int existingIdx = -1;
  for (int i = 0; i < (int)customTools.size(); ++i) {
      if (customTools[i].name == name) { existingIdx = i; break; }
  }
  if (existingIdx >= 0 && existingIdx < (int)userRegistry.size()) {
      userRegistry[existingIdx] = std::move(templateFig); // overwrite
  } else {
      int customId = (int)userRegistry.size();
      customTools.push_back({name, customId});
      userRegistry.push_back(std::move(templateFig));
  }

  m_polyStatusMsg = "Saved: " + path;
}

// ─── createConfiguredFigure (Circle / existing templates) ─────────────────────
std::unique_ptr<core::Figure> CreateFigureModal::createConfiguredFigure(
    const std::vector<std::unique_ptr<core::Figure>>& userRegistry)
{
  std::unique_ptr<core::Figure> fig;

  if (m_figureType == 0) {
    // Circle
    fig = std::make_unique<core::Circle>(m_radiusX, m_radiusY);
  } else if (m_figureType >= 2) {
    // Template (offset by 2: 0=Circle, 1=NewPolyline)
    int idx = m_figureType - 2;
    if (idx >= 0 && idx < (int)userRegistry.size()) {
      fig = userRegistry[idx]->clone();
      if (fig->typeName() == "polyline")
        static_cast<core::PolylineFigure*>(fig.get())->figureName = m_customName;
      else if (fig->typeName() == "composite")
        static_cast<core::CompositeFigure*>(fig.get())->figureName = m_customName;
    }
  }

  if (!fig) return nullptr;

  fig->fillColor = sf::Color(
      (sf::Uint8)(m_fillColor[0]*255), (sf::Uint8)(m_fillColor[1]*255),
      (sf::Uint8)(m_fillColor[2]*255), (sf::Uint8)(m_fillColor[3]*255));

  for (size_t i = 0; i < m_edges.size() && i < fig->edges.size(); ++i) {
    fig->edges[i].width = m_edges[i].width;
    fig->edges[i].color = sf::Color(
        (sf::Uint8)(m_edges[i].color[0]*255), (sf::Uint8)(m_edges[i].color[1]*255),
        (sf::Uint8)(m_edges[i].color[2]*255), (sf::Uint8)(m_edges[i].color[3]*255));
  }
  return fig;
}

// ─── render ───────────────────────────────────────────────────────────────────
void CreateFigureModal::render(core::Scene& scene,
    std::vector<Toolbar::CustomTool>& customTools,
    std::vector<std::unique_ptr<core::Figure>>& userRegistry)
{
  if (m_open)
    ImGui::OpenPopup("Create Figure##Modal");

  ImGui::SetNextWindowPos(
      ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f,
             ImGui::GetIO().DisplaySize.y * 0.5f),
      ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSizeConstraints(ImVec2(420, 300), ImVec2(560, 800));

  if (!ImGui::BeginPopupModal("Create Figure##Modal", &m_open,
                              ImGuiWindowFlags_AlwaysAutoResize))
    return;

  // ── Dropdown ────────────────────────────────────────────────────────────────
  int prevType = m_figureType;

  std::vector<const char*> allNames;
  allNames.push_back("Circle");
  allNames.push_back("New Polyline");
  // Separator-like label (selectable but disabled)
  bool hasTemplates = !customTools.empty();
  if (hasTemplates) allNames.push_back("──── Templates ────");
  for (auto& ct : customTools) allNames.push_back(ct.name.c_str());

  // We need custom combo because of the disabled separator entry
  if (ImGui::BeginCombo("Figure Type", allNames[m_figureType])) {
    // Circle
    if (ImGui::Selectable("Circle", m_figureType == 0)) m_figureType = 0;
    // New Polyline
    if (ImGui::Selectable("New Polyline", m_figureType == 1)) m_figureType = 1;
    // Templates header
    if (hasTemplates) {
      ImGui::Separator();
      ImGui::TextDisabled(" Templates");
      for (int i = 0; i < (int)customTools.size(); ++i) {
        bool sel = (m_figureType == i + 2);
        if (ImGui::Selectable(customTools[i].name.c_str(), sel))
          m_figureType = i + 2;
        if (sel) ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // Re-init edges if type changed
  if (m_figureType != prevType) {
    m_polyStatusMsg.clear();
    if (m_figureType == 0) {
      reinitEdges();
    } else if (m_figureType >= 2) {
      int idx = m_figureType - 2;
      if (idx < (int)userRegistry.size()) {
        const auto& rf = userRegistry[idx];
        m_edges.resize(rf->edges.size());
        auto lens = rf->getSideLengths();
        for (int i = 0; i < (int)m_edges.size(); ++i) {
          m_edges[i].length   = i < (int)lens.size() ? lens[i] : 100.f;
          m_edges[i].width    = rf->edges[i].width;
          m_edges[i].color[0] = rf->edges[i].color.r / 255.f;
          m_edges[i].color[1] = rf->edges[i].color.g / 255.f;
          m_edges[i].color[2] = rf->edges[i].color.b / 255.f;
          m_edges[i].color[3] = rf->edges[i].color.a / 255.f;
        }
        m_fillColor[0] = rf->fillColor.r / 255.f;
        m_fillColor[1] = rf->fillColor.g / 255.f;
        m_fillColor[2] = rf->fillColor.b / 255.f;
        m_fillColor[3] = rf->fillColor.a / 255.f;
        if (idx < (int)customTools.size())
          strncpy(m_customName, customTools[idx].name.c_str(), sizeof(m_customName)-1);
      }
    }
  }

  ImGui::Separator();

  // ══════════════════════════════════════════════════════════════════════════
  //  CIRCLE
  // ══════════════════════════════════════════════════════════════════════════
  if (m_figureType == 0) {
    ImGui::Text("Appearance");
    ImGui::ColorEdit4("Fill Color", m_fillColor);
    ImGui::Separator();
    ImGui::Text("Geometry");
    ImGui::DragFloat("Radius X", &m_radiusX, 1.f, 5.f, 2000.f);
    ImGui::DragFloat("Radius Y", &m_radiusY, 1.f, 5.f, 2000.f);
    ImGui::Separator();
    ImGui::Text("Outline");
    ImGui::PushID(0);
    ImGui::DragFloat("Thickness", &m_edges[0].width, 0.5f, 0.f, 100.f);
    ImGui::ColorEdit4("Color##e", m_edges[0].color, ImGuiColorEditFlags_NoInputs);
    ImGui::PopID();
    ImGui::Separator();

    if (ImGui::Button("Create", ImVec2(140, 0))) {
      auto fig = createConfiguredFigure(userRegistry);
      if (fig) {
        fig->anchor = scene.customOriginActive
            ? m_createPos - scene.customOriginPos
            : m_createPos;
        fig->parentOrigin = scene.customOriginActive
            ? scene.customOriginPos : sf::Vector2f(0.f,0.f);
        scene.setSelectedFigure(fig.get());
        scene.addFigure(std::move(fig));
      }
      m_open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      m_open = false; ImGui::CloseCurrentPopup();
    }
  }

  // ══════════════════════════════════════════════════════════════════════════
  //  NEW POLYLINE BUILDER
  // ══════════════════════════════════════════════════════════════════════════
  else if (m_figureType == 1) {
    ImGui::Text("Name / Template");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##PolyName", m_polyTemplateName, sizeof(m_polyTemplateName));

    ImGui::Separator();
    ImGui::Text("Appearance");
    ImGui::ColorEdit4("Fill##pf", m_polyFillColor);
    ImGui::DragFloat("Edge Width##pe", &m_polyEdgeWidth, 0.5f, 0.f, 50.f);
    ImGui::ColorEdit4("Edge Color##pc", m_polyEdgeColor, ImGuiColorEditFlags_NoInputs);

    ImGui::Separator();
    ImGui::Text("Segments  (length + direction angle)");

    // ── Segment table ──────────────────────────────────────────────────────
    ImGui::BeginChild("SegTable", ImVec2(0, std::min((int)m_polySegments.size()*28 + 36, 220)), true);
    ImGui::Columns(4, "segtbl", true);
    ImGui::SetColumnWidth(0, 36.f);
    ImGui::SetColumnWidth(1, 100.f);
    ImGui::SetColumnWidth(2, 100.f);
    ImGui::SetColumnWidth(3, 36.f);
    ImGui::TextDisabled("#"); ImGui::NextColumn();
    ImGui::TextDisabled("Length");   ImGui::NextColumn();
    ImGui::TextDisabled("Angle °");  ImGui::NextColumn();
    ImGui::TextDisabled("Del");      ImGui::NextColumn();
    ImGui::Separator();

    int toDelete = -1;
    for (int i = 0; i < (int)m_polySegments.size(); ++i) {
      ImGui::PushID(i);
      ImGui::Text("%d", i+1); ImGui::NextColumn();
      ImGui::SetNextItemWidth(-1.f);
      ImGui::DragFloat("##len", &m_polySegments[i].length, 1.f, 0.1f, 5000.f, "%.1f");
      ImGui::NextColumn();
      ImGui::SetNextItemWidth(-1.f);
      ImGui::DragFloat("##ang", &m_polySegments[i].angleDeg, 1.f, -360.f, 360.f, "%.1f");
      ImGui::NextColumn();
      if (ImGui::SmallButton("x")) toDelete = i;
      ImGui::NextColumn();
      ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    if (toDelete >= 0) m_polySegments.erase(m_polySegments.begin() + toDelete);

    if (ImGui::Button("+ Add Segment")) {
      PolySegment ps;
      // Default angle = same as last, convenient for chains
      if (!m_polySegments.empty()) ps.angleDeg = m_polySegments.back().angleDeg;
      m_polySegments.push_back(ps);
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All")) m_polySegments.clear();

    // ── Live preview: computed vertices ───────────────────────────────────
    if (!m_polySegments.empty()) {
      ImGui::Separator();
      if (ImGui::TreeNode("Preview Points")) {
        sf::Vector2f cur(0.f, 0.f);
        ImGui::TextDisabled("(0.0, 0.0)");
        for (auto& seg : m_polySegments) {
          float rad = seg.angleDeg * core::math::PI / 180.f;
          cur.x += seg.length * std::cos(rad);
          cur.y += seg.length * std::sin(rad);
          ImGui::TextDisabled("(%.1f, %.1f)", cur.x, cur.y);
        }
        ImGui::TreePop();
      }
    }

    // ── Status message ────────────────────────────────────────────────────
    if (!m_polyStatusMsg.empty()) {
      ImGui::TextColored(ImVec4(0.3f, 1.f, 0.3f, 1.f), "%s", m_polyStatusMsg.c_str());
    }

    ImGui::Separator();

    bool hasSegs = !m_polySegments.empty();
    if (!hasSegs) ImGui::BeginDisabled();

    // "Place Here" — build & add to scene at right-click position
    if (ImGui::Button("Place Here", ImVec2(120, 0))) {
      auto fig = buildPolylineFromSegments();
      if (fig) {
        fig->anchor = scene.customOriginActive
            ? m_createPos - scene.customOriginPos
            : m_createPos;
        fig->parentOrigin = scene.customOriginActive
            ? scene.customOriginPos : sf::Vector2f(0.f,0.f);
        scene.setSelectedFigure(fig.get());
        scene.addFigure(std::move(fig));
        m_open = false;
        ImGui::CloseCurrentPopup();
        if (!hasSegs) ImGui::EndDisabled();
        ImGui::EndPopup();
        return;
      }
    }
    ImGui::SameLine();
    // "Save + Place" — save template AND add to scene
    if (ImGui::Button("Save + Place", ImVec2(120, 0))) {
      savePolylineAsTemplate(customTools, userRegistry);
      if (m_polyStatusMsg.find("Saved") != std::string::npos) {
        // Also place the figure
        auto fig = buildPolylineFromSegments();
        if (fig) {
          fig->anchor = scene.customOriginActive
              ? m_createPos - scene.customOriginPos
              : m_createPos;
          fig->parentOrigin = scene.customOriginActive
              ? scene.customOriginPos : sf::Vector2f(0.f,0.f);
          scene.setSelectedFigure(fig.get());
          scene.addFigure(std::move(fig));
          m_open = false;
          ImGui::CloseCurrentPopup();
          if (!hasSegs) ImGui::EndDisabled();
          ImGui::EndPopup();
          return;
        }
      }
    }

    if (!hasSegs) ImGui::EndDisabled();

    ImGui::SameLine();
    // "Save as Template" only (no placement)
    if (ImGui::Button("Save Template", ImVec2(130, 0))) {
      savePolylineAsTemplate(customTools, userRegistry);
    }

    ImGui::Spacing();

    // "Draw on Canvas" — close modal and activate Polyline tool
    if (ImGui::Button("Draw on Canvas", ImVec2(200, 0))) {
      drawOnCanvasRequested = true;
      m_open = false;
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
      return;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      m_open = false; ImGui::CloseCurrentPopup();
    }
  }

  // ══════════════════════════════════════════════════════════════════════════
  //  EXISTING TEMPLATE
  // ══════════════════════════════════════════════════════════════════════════
  else {
    int idx = m_figureType - 2;
    if (idx >= 0 && idx < (int)customTools.size())
      ImGui::Text("Template: %s", customTools[idx].name.c_str());

    ImGui::InputText("Instance Name", m_customName, sizeof(m_customName));
    ImGui::ColorEdit4("Fill Color", m_fillColor);

    if (!m_edges.empty()) {
      ImGui::Separator();
      ImGui::Text("Edges: %zu", m_edges.size());
    }

    ImGui::Separator();
    if (ImGui::Button("Place", ImVec2(140, 0))) {
      auto fig = createConfiguredFigure(userRegistry);
      if (fig) {
        fig->anchor = scene.customOriginActive
            ? m_createPos - scene.customOriginPos
            : m_createPos;
        fig->parentOrigin = scene.customOriginActive
            ? scene.customOriginPos : sf::Vector2f(0.f,0.f);
        scene.setSelectedFigure(fig.get());
        scene.addFigure(std::move(fig));
      }
      m_open = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0))) {
      m_open = false; ImGui::CloseCurrentPopup();
    }
  }

  ImGui::EndPopup();
}

} // namespace ui
