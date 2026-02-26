#pragma once

#include "core/Figure.hpp"
#include <imgui.h>

namespace ui {

class PropertiesPanel {
public:
  PropertiesPanel() = default;

  // Renders the properties panel for a given figure.
  void render(core::Figure *selectedFigure);
};

} // namespace ui
