#include "SceneSerializer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

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
    if (auto cf = dynamic_cast<const CompositeFigure*>(fig)) {
        std::string pad(indent, ' ');
        out << pad << "figure composite\n";
        writeCommonFields(out, cf, indent);
        
        // Children
        out << pad << "children " << cf->children.size() << "\n";
        for (size_t i = 0; i < cf->children.size(); ++i) {
            out << pad << "  child_offset " << i << " " << cf->children[i].localOffset.x << " " << cf->children[i].localOffset.y << "\n";
            out << pad << "  child_rotation " << i << " " << cf->children[i].localRotation << "\n";
            out << pad << "  child_begin\n";
            writeFigure(out, cf->children[i].figure.get(), indent + 4);
            out << pad << "  child_end\n";
        }
        out << pad << "end\n";
    }
}

std::unique_ptr<Figure> SceneSerializer::readFigure(std::istream& in) {
    std::string type;
    if (!(in >> type)) return nullptr;
    
    // Legacy support or uniform CompositeFigure
    auto fig = std::make_unique<CompositeFigure>();
    fig->preset = CompositeFigure::Preset::None;
    
    // We treat everything as "composite" now. The old type string helps us map if it's an old save file.
    if (type == "rectangle") fig->preset = CompositeFigure::Preset::Rectangle;
    else if (type == "triangle") fig->preset = CompositeFigure::Preset::Triangle;
    else if (type == "hexagon") fig->preset = CompositeFigure::Preset::Hexagon;
    else if (type == "rhombus") fig->preset = CompositeFigure::Preset::Rhombus;
    else if (type == "trapezoid") fig->preset = CompositeFigure::Preset::Trapezoid;
    else if (type == "circle") fig->preset = CompositeFigure::Preset::Circle;
    
    readCommonFields(in, fig.get());
    return fig;
}

void SceneSerializer::writeCommonFields(std::ostream& out, const CompositeFigure* fig, int indent) {
    std::string pad(indent, ' ');
    out << pad << "name " << fig->figureName << "\n";
    out << pad << "preset " << static_cast<int>(fig->preset) << "\n";
    out << pad << "anchor " << fig->anchor.x << " " << fig->anchor.y << "\n";
    out << pad << "parent_origin " << fig->parentOrigin.x << " " << fig->parentOrigin.y << "\n";
    out << pad << "rotation " << fig->rotationAngle << "\n";
    out << pad << "scale " << fig->scale.x << " " << fig->scale.y << "\n";
    out << pad << "fill " << (int)fig->fillColor.r << " " << (int)fig->fillColor.g << " " 
        << (int)fig->fillColor.b << " " << (int)fig->fillColor.a << "\n";
    out << pad << "edges " << fig->edges.size() << "\n";
    for (size_t i = 0; i < fig->edges.size(); ++i) {
        out << pad << "  edge " << i << " width " << fig->edges[i].width << " color " 
            << (int)fig->edges[i].color.r << " " << (int)fig->edges[i].color.g << " "
            << (int)fig->edges[i].color.b << " " << (int)fig->edges[i].color.a << "\n";
    }
    
    // Convert base m_vertices through getVertices
    // Wait, getVertices() generates full absolute if it has children!
    // But we need the original `m_vertices` which is accessible via `getVerticesMutable` or we just cast away const
    // Actually `Figure` provides an override-able getVertices. Let's just use `getVerticesMutable()` for saving.
    auto verts = const_cast<CompositeFigure*>(fig)->getVerticesMutable();
    out << pad << "vertices_base " << verts.size() << "\n";
    for (size_t i = 0; i < verts.size(); ++i) {
        out << pad << "  vertex " << i << " " << verts[i].x << " " << verts[i].y << "\n";
    }
}

void SceneSerializer::readCommonFields(std::istream& in, CompositeFigure* fig) {
    std::string prop;
    while (in >> prop) {
        if (prop == "end") {
            break;
        } else if (prop == "name") {
            std::string name;
            in >> std::ws;
            std::getline(in, name);
            fig->figureName = name;
        } else if (prop == "preset") {
            int p;
            in >> p;
            fig->preset = static_cast<CompositeFigure::Preset>(p);
        } else if (prop == "anchor") {
            in >> fig->anchor.x >> fig->anchor.y;
        } else if (prop == "parent_origin") {
            in >> fig->parentOrigin.x >> fig->parentOrigin.y;
        } else if (prop == "rotation") {
            in >> fig->rotationAngle;
        } else if (prop == "scale") {
            in >> fig->scale.x >> fig->scale.y;
        } else if (prop == "fill") {
            int r, g, b, a;
            in >> r >> g >> b >> a;
            fig->fillColor = sf::Color(r, g, b, a);
        } else if (prop == "edges") {
            size_t count;
            in >> count;
            fig->edges.resize(count);
            for (size_t i = 0; i < count; ++i) {
                std::string dummy;
                size_t idx;
                float width;
                int r, g, b, a;
                in >> dummy >> idx >> dummy >> width >> dummy >> r >> g >> b >> a;
                if (idx < count) {
                    fig->edges[idx].width = width;
                    fig->edges[idx].color = sf::Color(r, g, b, a);
                }
            }
        } else if (prop == "vertices" || prop == "vertices_base") {
            size_t count;
            in >> count;
            auto& m_verts = fig->getVerticesMutable();
            m_verts.resize(count);
            for (size_t i = 0; i < count; ++i) {
                std::string dummy;
                size_t idx;
                float px, py;
                in >> dummy >> idx >> px >> py;
                if (idx < count) {
                    m_verts[idx] = {px, py};
                }
            }
        } else if (prop == "children") {
            size_t childCount;
            in >> childCount;
            for (size_t i = 0; i < childCount; ++i) {
                CompositeFigure::Child child;
                std::string word;
                while (in >> word) {
                    if (word == "child_offset") {
                        size_t idx;
                        in >> idx >> child.localOffset.x >> child.localOffset.y;
                    } else if (word == "child_rotation") {
                        size_t idx;
                        in >> idx >> child.localRotation;
                    } else if (word == "child_begin") {
                        std::string figWord;
                        in >> figWord; 
                        if (figWord == "figure") {
                            child.figure = readFigure(in);
                        }
                    } else if (word == "child_end") {
                        if (child.figure) fig->children.push_back(std::move(child));
                        break;
                    }
                }
            }
        }
    }
}

} // namespace core
