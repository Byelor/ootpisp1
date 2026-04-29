#pragma once

#include <imgui.h>
#include <vector>
#include <string>

namespace core { class Scene; }

namespace ui {

enum class Tool {
  Select,
  Circle,
  Polyline,
  CompoundSelect,
  Custom
};

class Toolbar {
public:
  Toolbar() = default;

  struct CustomTool {
      std::string name;
      int customId;
  };

  std::vector<CustomTool> customTools;
  bool showCustomFigManager = false;

  // Renders the toolbar and returns true if the tool changed
  bool render(Tool &currentTool, core::Scene& scene, int& selectedCustomToolId);
};

} // namespace ui
