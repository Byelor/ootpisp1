#pragma once

#include "../core/Scene.hpp"
#include "../core/Viewport.hpp"
#include <imgui.h>

namespace ui {

class PropertiesPanel {
public:
  // Renders the properties panel and returns true if "Fit to Screen" was
  // requested
  bool render(core::Scene &scene, core::Viewport &viewport);

private:
  bool m_lockProportions = true;

public:
  bool m_lockAnchor = false;
  bool m_drawOriginsOverFigures = true;
};

} // namespace ui
