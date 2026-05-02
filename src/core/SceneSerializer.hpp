#pragma once
#include "Scene.hpp"
#include "CompositeFigure.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>

namespace core {

class SceneSerializer {
public:
    static bool save(const Scene& scene, const std::string& filepath);
    static bool load(Scene& scene, const std::string& filepath);
    
    static nlohmann::json writeFigureJson(const Figure* fig);
    static std::unique_ptr<Figure> readFigureJson(const nlohmann::json& j);

    // Custom figure template support
    // Save a single figure as a template to a .json file
    static bool saveFigureTemplate(const Figure* fig, const std::string& filepath);

    // Load a single figure from a .json template file
    static std::unique_ptr<Figure> loadFigureTemplate(const std::string& filepath);

    // List all .json files in a directory (returns full paths)
    static std::vector<std::string> listFigureTemplates(const std::string& dir);
};

} // namespace core
