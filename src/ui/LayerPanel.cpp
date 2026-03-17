#include "LayerPanel.hpp"
#include <imgui.h>
#include <iostream>
#include "../core/MathUtils.hpp"

namespace ui {

bool LayerPanel::render(core::Scene& scene) {
    bool changed = false;
    m_wantsDrop = false;

    ImGui::SetNextWindowPos(ImVec2(0, 60), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, ImGui::GetIO().DisplaySize.y - 90), ImGuiCond_Always);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Layers Flow", nullptr, flags);

    if (ImGui::Button("Deselect All")) {
        scene.setSelectedFigure(nullptr);
    }

    ImGui::Separator();

    // The root drop target (for placing figures back into the main scene at the very end)
    ImGui::BeginChild("LayerTree", ImVec2(0, 0), true);

    for (int i = 0; i < scene.figureCount(); ++i) {
        core::Figure* fig = scene.getFigure(i);
        renderFigureNode(scene, fig, i, nullptr);
    }

    // Dummy payload drop space at the bottom to drag figures to the root end
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 50.f));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FIGURE_PAYLOAD")) {
            m_dropTargetFig = nullptr;
            m_dropTargetParent = nullptr;
            m_dropTargetIndex = scene.figureCount();
            m_wantsDrop = true;
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();

    ImGui::End();

    // Resolve drop
    if (m_wantsDrop && m_dragSourceFig) {
        std::unique_ptr<core::Figure> extracted;
        
        // 1. Calculate absolute properties before extraction
        sf::Vector2f absPos = m_dragSourceFig->parentOrigin + m_dragSourceFig->anchor;
        float absRot = m_dragSourceFig->rotationAngle;
        
        // 2. Extract
        if (m_dragSourceParent) {
            extracted = m_dragSourceParent->extractChild(m_dragSourceFig);
        } else {
            extracted = scene.extractFigure(m_dragSourceFig);
        }

        if (extracted) {
            // 3. Insert into target
            if (m_dropTargetParent) {
                // Determine new local properties relative to the new parent
                // newParent is m_dropTargetParent
                sf::Vector2f parentAbsPos = m_dropTargetParent->parentOrigin + m_dropTargetParent->anchor;
                float parentRot = m_dropTargetParent->rotationAngle;
                
                sf::Vector2f diff = absPos - parentAbsPos;
                // Rotate diff by -parentRot to get local offset
                sf::Vector2f localOffset = core::math::rotate(diff, -parentRot * core::math::DEG_TO_RAD);
                // Also account for parent scale if it varies, but usually it's 1, 1 if we applyScale()
                if (m_dropTargetParent->scale.x != 0 && m_dropTargetParent->scale.y != 0) {
                    localOffset.x /= m_dropTargetParent->scale.x;
                    localOffset.y /= m_dropTargetParent->scale.y;
                }
                
                float localRot = absRot - parentRot;

                m_dropTargetParent->insertChild(std::move(extracted), m_dropTargetIndex, localOffset, localRot);
            } else {
                // Drop to scene root
                extracted->parentOrigin = sf::Vector2f(0.f, 0.f);
                extracted->anchor = absPos;
                extracted->rotationAngle = absRot;
                scene.insertFigure(std::move(extracted), m_dropTargetIndex);
            }
            changed = true;
        }
        
        m_dragSourceFig = nullptr;
        m_dragSourceParent = nullptr;
        m_wantsDrop = false;
    }

    return changed;
}

void LayerPanel::renderFigureNode(core::Scene& scene, core::Figure* fig, int index, core::CompositeFigure* parent) {
    if (!fig) return;

    core::CompositeFigure* comp = dynamic_cast<core::CompositeFigure*>(fig);
    bool isGroup = comp && !comp->children.empty();
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (scene.getSelectedFigure() == fig) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    if (!isGroup) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    std::string typeName = fig->typeName();
    std::string label = typeName + "##" + std::to_string(reinterpret_cast<uintptr_t>(fig));

    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        scene.setSelectedFigure(fig);
    }

    // Drag Source
    if (ImGui::BeginDragDropSource()) {
        m_dragSourceFig = fig;
        m_dragSourceParent = parent;
        ImGui::SetDragDropPayload("FIGURE_PAYLOAD", &fig, sizeof(void*));
        ImGui::Text("Moving %s", typeName.c_str());
        ImGui::EndDragDropSource();
    }

    // Drag Target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FIGURE_PAYLOAD")) {
            m_dropTargetFig = fig;
            m_dropTargetParent = parent;
            m_dropTargetIndex = index;
            m_wantsDrop = true;
        }
        ImGui::EndDragDropTarget();
    }

    if (isGroup) {
        // If it's a group, we can also let users drop directly INTO the group (at the end of children) by dropping on the node itself
        if (ImGui::BeginDragDropTarget()) {
             if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FIGURE_PAYLOAD_TO_GROUP")) { // Fallback standard
             }
             ImGui::EndDragDropTarget();
        }
        
        if (nodeOpen) {
            // Render children
            for (size_t i = 0; i < comp->children.size(); ++i) {
                renderFigureNode(scene, comp->children[i].figure.get(), i, comp);
            }
            
            // Dummy target to drop at the end of this group's children list
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 4.f));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FIGURE_PAYLOAD")) {
                    m_dropTargetFig = nullptr;
                    m_dropTargetParent = comp;
                    m_dropTargetIndex = comp->children.size();
                    m_wantsDrop = true;
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::TreePop();
        }
    }
}

} // namespace ui
