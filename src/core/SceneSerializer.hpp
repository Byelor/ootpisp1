#pragma once
#include "Scene.hpp"
#include "CompositeFigure.hpp"
#include <string>
#include <iostream>
#include <memory>
#include <vector>

namespace core {

class SceneSerializer {
public:
    static bool save(const Scene& scene, const std::string& filepath);
    static bool load(Scene& scene, const std::string& filepath);
    
    static void writeFigure(std::ostream& out, const Figure* fig, int indent = 0);
    static std::unique_ptr<Figure> readFigure(std::istream& in);

    // Custom figure template support
    // Save a single figure as a template to a .fig file
    static bool saveFigureTemplate(const Figure* fig, const std::string& filepath);

    // Load a single figure from a .fig template file
    static std::unique_ptr<Figure> loadFigureTemplate(const std::string& filepath);

    // List all .fig files in a directory (returns full paths)
    static std::vector<std::string> listFigureTemplates(const std::string& dir);
};

} // namespace core
