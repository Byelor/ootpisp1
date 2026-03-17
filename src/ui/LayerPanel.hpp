#pragma once

#include "../core/Scene.hpp"
#include "../core/CompositeFigure.hpp"

namespace ui {

class LayerPanel {
public:
    LayerPanel() = default;

    // Renders the layers panel. Returns true if something changed (reordered, grouped, etc.)
    bool render(core::Scene& scene);

private:
    void renderFigureNode(core::Scene& scene, core::Figure* fig, int index, core::CompositeFigure* parent);

    // DND state holding
    core::Figure* m_dragSourceFig = nullptr;
    core::CompositeFigure* m_dragSourceParent = nullptr;
    
    bool m_wantsDrop = false;
    core::Figure* m_dropTargetFig = nullptr;
    core::CompositeFigure* m_dropTargetParent = nullptr;
    int m_dropTargetIndex = -1;
};

} // namespace ui
