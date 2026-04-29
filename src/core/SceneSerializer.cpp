#include "SceneSerializer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <filesystem>
#include "Figures.hpp"
#include "PolylineFigure.hpp"

namespace fs = std::filesystem;

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
    
    // Factory — only circle, polyline and composite remain
    if (type == "circle") fig = std::make_unique<core::Circle>(50.f, 50.f);
    else if (type == "polyline") fig = std::make_unique<core::PolylineFigure>();
    else if (type == "composite") fig = std::make_unique<core::CompositeFigure>();
    else {
        // Unknown type (e.g. old rectangle/triangle) — skip until "end"
        std::cerr << "[SceneSerializer] Unknown figure type '" << type << "', skipping.\n";
        std::string prop;
        while (in >> prop) {
            if (prop == "end") break;
            // consume rest of line
            std::string dummy;
            std::getline(in, dummy);
        }
        return nullptr;
    }
    
    std::string prop;
    while (in >> prop) {
        if (prop == "end") {
            break;
        }
        fig->deserialize(prop, in);
    }
    return fig;
}

// ─── Template API ────────────────────────────────────────────────────────────

bool SceneSerializer::saveFigureTemplate(const Figure* fig, const std::string& filepath) {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    writeFigure(out, fig, 0);
    return true;
}

std::unique_ptr<Figure> SceneSerializer::loadFigureTemplate(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return nullptr;
    std::string word;
    while (in >> word) {
        if (word == "figure") {
            return readFigure(in);
        }
    }
    return nullptr;
}

std::vector<std::string> SceneSerializer::listFigureTemplates(const std::string& dir) {
    std::vector<std::string> result;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return result;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".fig") {
            result.push_back(entry.path().string());
        }
    }
    // Sort alphabetically for consistent ordering
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace core
