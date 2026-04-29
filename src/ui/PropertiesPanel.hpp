#pragma once

#include "core/Scene.hpp"
#include "core/Viewport.hpp"
#include "Toolbar.hpp"
#include <imgui.h>
#include <vector>
#include <memory>

namespace ui {

class PropertiesPanel {
public:
  // Renders the properties panel and returns true if "Fit to Screen" was
  // requested
  bool render(core::Scene &scene, core::Viewport &viewport, std::vector<core::Figure*>& compoundSelection, std::vector<std::unique_ptr<core::Figure>>& userRegistry, Toolbar& toolbar);

private:
  bool m_lockProportions = true;
  char m_templateName[256] = "";
  std::string m_templateSaveStatus;

public:
  bool m_lockAnchor = false;
  bool m_drawOriginsOverFigures = true;
};

} // namespace ui
