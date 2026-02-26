#pragma once

#include <imgui.h>

namespace ui {

enum class Tool { Select, Rectangle, Triangle, Hexagon, Rhombus, Trapezoid };

class Toolbar {
public:
  Toolbar() = default;

  // Renders the toolbar and returns true if the tool changed
  bool render(Tool &currentTool);
};

} // namespace ui
