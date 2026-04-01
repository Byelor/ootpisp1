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

static void writePolylineFields(std::ostream& out, const PolylineFigure* fig, int indent);
static void readPolylineFields(std::istream& in, PolylineFigure* fig);

static void writeCircleFields(std::ostream& out, const Circle* fig, int indent);
static void readCircleFields(std::istream& in, Circle* fig);

void SceneSerializer::writeFigure(std::ostream& out, const Figure* fig, int indent) {
    std::string pad(indent, ' ');
    if (auto cf = dynamic_cast<const CompositeFigure*>(fig)) {
        out << pad << "figure " << cf->typeName() << "\n";
        writeCommonFields(out, cf, indent);
        
        // Children
        out << pad << "children " << cf->children.size() << "\n";
        for (size_t i = 0; i < cf->children.size(); ++i) {
            out << pad << "  child_offset " << i << " " << cf->children[i].figure->anchor.x << " " << cf->children[i].figure->anchor.y << "\n";
            out << pad << "  child_rotation " << i << " " << cf->children[i].figure->rotationAngle << "\n";
            out << pad << "  child_begin\n";
            writeFigure(out, cf->children[i].figure.get(), indent + 4);
            out << pad << "  child_end\n";
        }
        out << pad << "end\n";
    } else if (auto circ = dynamic_cast<const Circle*>(fig)) {
        out << pad << "figure " << circ->typeName() << "\n";
        writeCircleFields(out, circ, indent);
        out << pad << "end\n";
    } else if (auto pf = dynamic_cast<const PolylineFigure*>(fig)) {
        out << pad << "figure " << pf->typeName() << "\n";
        writePolylineFields(out, pf, indent);
        out << pad << "end\n";
    }
}

static void writePolylineFields(std::ostream& out, const PolylineFigure* fig, int indent) {
    std::string pad(indent, ' ');
    out << pad << "id " << fig->id << "\n";
    out << pad << "name " << fig->figureName << "\n";
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
    auto verts = const_cast<PolylineFigure*>(fig)->getVerticesMutable();
    out << pad << "vertices_base " << verts.size() << "\n";
    for (size_t i = 0; i < verts.size(); ++i) {
        out << pad << "  vertex " << i << " " << verts[i].x << " " << verts[i].y << "\n";
    }
}

static void readPolylineFields(std::istream& in, PolylineFigure* fig) {
    std::string prop;
    while (in >> prop) {
        if (prop == "end") break;
        else if (prop == "id") in >> fig->id;
        else if (prop == "name") {
            in >> std::ws;
            std::getline(in, fig->figureName);
        }
        else if (prop == "anchor") in >> fig->anchor.x >> fig->anchor.y;
        else if (prop == "parent_origin") in >> fig->parentOrigin.x >> fig->parentOrigin.y;
        else if (prop == "rotation") in >> fig->rotationAngle;
        else if (prop == "scale") in >> fig->scale.x >> fig->scale.y;
        else if (prop == "fill") {
            int r, g, b, a; in >> r >> g >> b >> a;
            fig->fillColor = sf::Color(r, g, b, a);
        } else if (prop == "edges") {
            size_t count; in >> count;
            fig->edges.resize(count);
            for (size_t i = 0; i < count; ++i) {
                std::string dummy; size_t idx; float width; int r, g, b, a;
                in >> dummy >> idx >> dummy >> width >> dummy >> r >> g >> b >> a;
                if (idx < count) {
                    fig->edges[idx].width = width;
                    fig->edges[idx].color = sf::Color(r, g, b, a);
                }
            }
        } else if (prop == "vertices" || prop == "vertices_base") {
            size_t count; in >> count;
            auto& m_verts = fig->getVerticesMutable();
            m_verts.resize(count);
            for (size_t i = 0; i < count; ++i) {
                std::string dummy; size_t idx; float px, py;
                in >> dummy >> idx >> px >> py;
                if (idx < count) m_verts[idx] = {px, py};
            }
        }
    }
}

std::unique_ptr<Figure> SceneSerializer::readFigure(std::istream& in) {
    std::string type;
    if (!(in >> type)) return nullptr;
    
    std::unique_ptr<Figure> fig;
    
    if (type == "rectangle") fig = std::make_unique<core::Rectangle>(100.f, 100.f);
    else if (type == "triangle") fig = std::make_unique<core::Triangle>(100.f, 100.f);
    else if (type == "hexagon") fig = std::make_unique<core::Hexagon>(100.f, 100.f);
    else if (type == "rhombus") fig = std::make_unique<core::Rhombus>(100.f, 100.f);
    else if (type == "trapezoid") fig = std::make_unique<core::Trapezoid>(60.f, 100.f, 100.f);
    else if (type == "circle") fig = std::make_unique<core::Circle>(50.f, 50.f);
    else if (type == "polyline") fig = std::make_unique<core::PolylineFigure>();
    else fig = std::make_unique<core::CompositeFigure>(); // fallback or explicit "composite"
    
    if (auto cf = dynamic_cast<CompositeFigure*>(fig.get())) {
        readCommonFields(in, cf);
    } else if (auto circ = dynamic_cast<Circle*>(fig.get())) {
        readCircleFields(in, circ);
    } else if (auto pf = dynamic_cast<PolylineFigure*>(fig.get())) {
        readPolylineFields(in, pf);
    }
    return fig;
}

void SceneSerializer::writeCommonFields(std::ostream& out, const CompositeFigure* fig, int indent) {
    std::string pad(indent, ' ');
    out << pad << "id " << fig->id << "\n";
    out << pad << "name " << fig->figureName << "\n";
    out << pad << "solid_group " << (fig->isSolidGroup ? 1 : 0) << "\n";
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
        } else if (prop == "id") {
            in >> fig->id;
        } else if (prop == "name") {
            std::string name;
            in >> std::ws;
            std::getline(in, name);
            fig->figureName = name;
        } else if (prop == "preset") {
            int p;
            in >> p;
            // Preset logic removed, ignore this legacy property
        } else if (prop == "anchor") {
            in >> fig->anchor.x >> fig->anchor.y;
        } else if (prop == "parent_origin") {
            in >> fig->parentOrigin.x >> fig->parentOrigin.y;
        } else if (prop == "rotation") {
            in >> fig->rotationAngle;
        } else if (prop == "scale") {
            in >> fig->scale.x >> fig->scale.y;
        } else if (prop == "solid_group") {
            int v; in >> v;
            fig->isSolidGroup = (v != 0);
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
                        in >> idx >> child.figure->anchor.x >> child.figure->anchor.y;
                    } else if (word == "child_rotation") {
                        size_t idx;
                        in >> idx >> child.figure->rotationAngle;
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

static void writeCircleFields(std::ostream& out, const Circle* fig, int indent) {
    std::string pad(indent, ' ');
    out << pad << "id " << fig->id << "\n";
    out << pad << "name " << fig->figureName << "\n";
    out << pad << "anchor " << fig->anchor.x << " " << fig->anchor.y << "\n";
    out << pad << "parent_origin " << fig->parentOrigin.x << " " << fig->parentOrigin.y << "\n";
    out << pad << "rotation " << fig->rotationAngle << "\n";
    out << pad << "scale " << fig->scale.x << " " << fig->scale.y << "\n";
    out << pad << "fill " << (int)fig->fillColor.r << " " << (int)fig->fillColor.g << " " 
        << (int)fig->fillColor.b << " " << (int)fig->fillColor.a << "\n";
    if (!fig->edges.empty()) {
        out << pad << "edge_width " << fig->edges[0].width << "\n";
        out << pad << "edge_color " << (int)fig->edges[0].color.r << " " << (int)fig->edges[0].color.g << " "
            << (int)fig->edges[0].color.b << " " << (int)fig->edges[0].color.a << "\n";
    }
    out << pad << "radius " << fig->getRadiusX() << " " << fig->getRadiusY() << "\n";
}

static void readCircleFields(std::istream& in, Circle* fig) {
    std::string prop;
    float rx = 50.f, ry = 50.f;
    fig->edges.resize(1); // Circle always has 1 virtual edge for thickness
    while (in >> prop) {
        if (prop == "end") break;
        else if (prop == "id") in >> fig->id;
        else if (prop == "name") {
            in >> std::ws;
            std::getline(in, fig->figureName);
        }
        else if (prop == "anchor") in >> fig->anchor.x >> fig->anchor.y;
        else if (prop == "parent_origin") in >> fig->parentOrigin.x >> fig->parentOrigin.y;
        else if (prop == "rotation") in >> fig->rotationAngle;
        else if (prop == "scale") in >> fig->scale.x >> fig->scale.y;
        else if (prop == "fill") {
            int r, g, b, a; in >> r >> g >> b >> a;
            fig->fillColor = sf::Color(r, g, b, a);
        } else if (prop == "edge_width") {
            in >> fig->edges[0].width;
        } else if (prop == "edge_color") {
            int r, g, b, a; in >> r >> g >> b >> a;
            fig->edges[0].color = sf::Color(r, g, b, a);
        } else if (prop == "radius") {
            in >> rx >> ry;
        } else if (prop == "edges" || prop == "vertices" || prop == "vertices_base") {
            // Legacy polygon fields, ignore them for new circles
            std::string line; std::getline(in, line); 
        }
    }
    fig->setRadius(rx, ry);
}

} // namespace core
