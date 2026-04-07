#pragma once
#include "Scene.hpp"
#include "CompositeFigure.hpp"
#include <string>
#include <iostream>
#include <memory>

namespace core {

class SceneSerializer {
public:
    static bool save(const Scene& scene, const std::string& filepath);
    static bool load(Scene& scene, const std::string& filepath);
    
    static void writeFigure(std::ostream& out, const Figure* fig, int indent = 0);
    static std::unique_ptr<Figure> readFigure(std::istream& in);
};

} // namespace core
