#pragma once

#include "../core/Figure.hpp"
#include "../core/Viewport.hpp"
#include <imgui.h>

namespace ui {

class PropertiesPanel {
public:
  // Renders the properties panel and returns true if "Fit to Screen" was
  // requested
  bool render(core::Figure *selectedFigure, core::Viewport &viewport);

private:
  bool m_lockProportions = true;
};

} // namespace ui
