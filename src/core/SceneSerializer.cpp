#include "SceneSerializer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include "Figures.hpp"
#include "PolylineFigure.hpp"

namespace core {

bool SceneSerializer::save(const Scene& scene, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    for (int i = 0; i < scene.figureCount(); ++i) {
        writeFigure(out, scene.getFigure(i), 0);
        out << "\n";
    }
    return true;
}

bool SceneSerializer::load(Scene& scene, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;
    std::string word;
    while (in >> word) {
        if (word == "figure") {
            auto fig = readFigure(in);
            if (fig) scene.addFigure(std::move(fig));
        }
    }
    return true;
}

void SceneSerializer::writeFigure(std::ostream& out, const Figure* fig, int indent) {
    std::string pad(indent, ' ');
    out << pad << "figure " << fig->typeName() << "\n";
    fig->serialize(out, indent);
    out << pad << "end\n";
}

std::unique_ptr<Figure> SceneSerializer::readFigure(std::istream& in) {
    std::string type;
    if (!(in >> type)) return nullptr;
    
    std::unique_ptr<Figure> fig;
    
    // Factory
    if (type == "rectangle") fig = std::make_unique<core::Rectangle>(100.f, 100.f);
    else if (type == "triangle") fig = std::make_unique<core::Triangle>(100.f, 100.f);
    else if (type == "hexagon") fig = std::make_unique<core::Hexagon>(100.f, 100.f);
    else if (type == "rhombus") fig = std::make_unique<core::Rhombus>(100.f, 100.f);
    else if (type == "trapezoid") fig = std::make_unique<core::Trapezoid>(60.f, 100.f, 100.f);
    else if (type == "circle") fig = std::make_unique<core::Circle>(50.f, 50.f);
    else if (type == "polyline") fig = std::make_unique<core::PolylineFigure>();
    else fig = std::make_unique<core::CompositeFigure>(); // fallback or explicit "composite"
    
    std::string prop;
    while (in >> prop) {
        if (prop == "end") {
            break;
        }
        fig->deserialize(prop, in);
    }
    return fig;
}

} // namespace core
