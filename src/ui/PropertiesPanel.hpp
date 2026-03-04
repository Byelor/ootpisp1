#pragma once

#include "../core/Scene.hpp"
#include "../core/Viewport.hpp"
#include <array>
#include <imgui.h>

namespace ui {

class PropertiesPanel {
public:
  // Renders the properties panel and returns true if "Fit to Screen" was
  // requested
  bool render(core::Scene &scene, core::Viewport &viewport);

private:
  bool m_lockProportions = true;
  // Per-side lock state (max 8 sides, reset when figure changes)
  std::array<bool, 8> m_lockedSides = {};
  core::Figure *m_lastFigure = nullptr;

public:
  bool m_lockAnchor = false;
  bool m_drawOriginsOverFigures = true;
};

} // namespace ui
