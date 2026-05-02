#include "SceneSerializer.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>
#include "Figures.hpp"
#include "PolylineFigure.hpp"

namespace fs = std::filesystem;

namespace core {

bool SceneSerializer::save(const Scene& scene, const std::string& filepath) {
    nlohmann::json root;
    nlohmann::json figuresArr = nlohmann::json::array();
    for (int i = 0; i < scene.figureCount(); ++i) {
        figuresArr.push_back(writeFigureJson(scene.getFigure(i)));
    }
    root["figures"] = figuresArr;

    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << root.dump(2);
    return true;
}

bool SceneSerializer::load(Scene& scene, const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return false;

    nlohmann::json root;
    try {
        in >> root;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[SceneSerializer] JSON parse error: " << e.what() << "\n";
        return false;
    }

    if (root.contains("figures")) {
        for (const auto& fj : root["figures"]) {
            auto fig = readFigureJson(fj);
            if (fig) scene.addFigure(std::move(fig));
        }
    }
    return true;
}

nlohmann::json SceneSerializer::writeFigureJson(const Figure* fig) {
    nlohmann::json j = fig->serializeToJson();
    j["type"] = fig->typeName();
    return j;
}

std::unique_ptr<Figure> SceneSerializer::readFigureJson(const nlohmann::json& j) {
    std::string type = j.value("type", "");

    std::unique_ptr<Figure> fig;

    if (type == "circle") fig = std::make_unique<core::Circle>(50.f, 50.f);
    else if (type == "polyline") fig = std::make_unique<core::PolylineFigure>();
    else if (type == "composite") fig = std::make_unique<core::CompositeFigure>();
    else {
        std::cerr << "[SceneSerializer] Unknown figure type '" << type << "', skipping.\n";
        return nullptr;
    }

    fig->deserializeFromJson(j);
    return fig;
}

// ─── Template API ────────────────────────────────────────────────────────────

bool SceneSerializer::saveFigureTemplate(const Figure* fig, const std::string& filepath) {
    nlohmann::json j = writeFigureJson(fig);

    std::ofstream out(filepath);
    if (!out.is_open()) return false;
    out << j.dump(2);
    return true;
}

std::unique_ptr<Figure> SceneSerializer::loadFigureTemplate(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) return nullptr;

    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[SceneSerializer] JSON parse error in " << filepath << ": " << e.what() << "\n";
        return nullptr;
    }

    return readFigureJson(j);
}

std::vector<std::string> SceneSerializer::listFigureTemplates(const std::string& dir) {
    std::vector<std::string> result;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return result;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            result.push_back(entry.path().string());
        }
    }
    // Sort alphabetically for consistent ordering
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace core
