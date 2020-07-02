#pragma once

#include "coordinates.h"
#include "pvl/Optional.hpp"
#include "pvl/Vector.hpp"
#include <chrono>
#include <iostream>
#include <istream>
#include <sstream>
#include <vector>

using Color = Pvl::Vector<uint8_t, 3>;

struct Mesh {
    using Face = std::array<uint32_t, 3>;

    // in local coords
    std::vector<Pvl::Vec3f> vertices;
    std::vector<Pvl::Vec3f> normals;
    std::vector<Color> colors;
    std::vector<Face> faces;
    Srs srs;

    Pvl::Vec3f normal(const uint32_t fi) const {
        Pvl::Vec3f p0 = vertices[faces[fi][0]];
        Pvl::Vec3f p1 = vertices[faces[fi][1]];
        Pvl::Vec3f p2 = vertices[faces[fi][2]];

        Pvl::Vec3f n = Pvl::crossProd(p1 - p0, p2 - p0);
        float len = Pvl::norm(n);
        if (len > 1.e-20) {
            return n / len;
        } else {
            return Pvl::Vec3f(0, 0, 1);
        }
    }
    float area(const uint32_t fi) const {
        Pvl::Vec3f p0 = vertices[faces[fi][0]];
        Pvl::Vec3f p1 = vertices[faces[fi][1]];
        Pvl::Vec3f p2 = vertices[faces[fi][2]];

        return 0.5f * Pvl::norm(Pvl::crossProd(p1 - p0, p2 - p0));
    }
};


inline void savePly(std::ostream& out, const Mesh& mesh) {
    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "comment Created by MPCV\n";
    out << "element vertex " << mesh.vertices.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    out << "element face " << mesh.faces.size() << "\n";
    out << "property list uchar int vertex_index\n";
    out << "end_header\n";
    for (const Pvl::Vec3f& p : mesh.vertices) {
        out << p[0] << " " << p[1] << " " << p[2] << "\n";
    }
    for (const Mesh::Face& f : mesh.faces) {
        out << "3 " << f[0] << " " << f[1] << " " << f[2] << "\n";
    }
}

template <typename Progress>
inline Mesh loadPly(std::istream& in, const Progress& prog) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::string line;
    std::size_t numVertices = 0;
    std::size_t numFaces = 0;
    char prop[256];

    int propIdx = 0;
    int normalProp = -1;
    int colorProp = -1;
    while (std::getline(in, line)) {
        sscanf(line.c_str(), "element vertex %zu", &numVertices);
        sscanf(line.c_str(), "element face %zu", &numFaces);
        sscanf(line.c_str(), "property float %s", prop);
        if (std::string(prop) == "x") {
            propIdx++;
        }
        if (std::string(prop) == "nx") {
            normalProp = propIdx++;
        }
        sscanf(line.c_str(), "property uchar %s", prop);
        if (std::string(prop) == "red") {
            colorProp = propIdx++;
        }

        if (line == "end_header") {
            break;
        }
    }
    std::cout << "Loading mesh with " << numVertices << " vertices and " << numFaces << " faces" << std::endl;
    if (normalProp != -1) {
        std::cout << "Has point normals" << std::endl;
    }
    if (colorProp != -1) {
        std::cout << "Has point colors" << std::endl;
    }

    Mesh mesh;
    mesh.vertices.reserve(numVertices);
    mesh.faces.reserve(numFaces);
    if (normalProp != -1) {
        mesh.normals.reserve(numVertices);
    }
    if (colorProp != -1) {
        mesh.colors.reserve(numVertices);
    }


    const int progStep = std::max((numVertices + numFaces) / 100, std::size_t(100));
    std::size_t nextProg = progStep;
    float indexToProg = 100.f / (numVertices + numFaces);
    for (std::size_t i = 0; i < numVertices; ++i) {
        std::getline(in, line);
        /// \todo simplify
        Pvl::Vec3f p;
        if (normalProp == 1 && colorProp == 2) {
            Pvl::Vec3f n;
            Color c;
            sscanf(line.c_str(),
                "%f%f%f%f%f%f%hhu%hhu%hhu",
                &p[0],
                &p[1],
                &p[2],
                &n[0],
                &n[1],
                &n[2],
                &c[0],
                &c[1],
                &c[2]);
            mesh.vertices.push_back(p);
            mesh.normals.push_back(n);
            mesh.colors.push_back(c);
        } else if (normalProp == 1) {
            Pvl::Vec3f n;
            sscanf(line.c_str(), "%f%f%f%f%f%f", &p[0], &p[1], &p[2], &n[0], &n[1], &n[2]);
            mesh.vertices.push_back(p);
            mesh.normals.push_back(n);
        } else if (colorProp == 1) {
            Color c;
            sscanf(line.c_str(), "%f%f%f%hhu%hhu%hhu", &p[0], &p[1], &p[2], &c[0], &c[1], &c[2]);
            mesh.vertices.push_back(p);
            mesh.colors.push_back(c);
        } else {
            sscanf(line.c_str(), "%f%f%f", &p[0], &p[1], &p[2]);
            mesh.vertices.push_back(p);
        }

        if (i == nextProg) {
            if (prog(i * indexToProg)) {
                return {};
            }
            nextProg += progStep;
        }
    }
    std::cout << "Added " << mesh.vertices.size() << " vertices " << std::endl;
    nextProg = progStep;
    for (std::size_t i = 0; i < numFaces; ++i) {
        std::getline(in, line);
        int dummy;
        Mesh::Face f;
        sscanf(line.c_str(), "%d%d%d%d", &dummy, &f[0], &f[1], &f[2]);
        mesh.faces.emplace_back(f);

        if (i == nextProg) {
            if (prog((i + numVertices) * indexToProg)) {
                return {}; // Pvl::NONE;
            }
            nextProg += progStep;
        }
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Ply mesh loaded in  "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms"
              << std::endl;
    return mesh;
}
