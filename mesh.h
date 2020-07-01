#pragma once

#include "pvl/Optional.hpp"
#include "pvl/Vector.hpp"
#include <chrono>
#include <iostream>
#include <istream>
#include <sstream>
#include <vector>

struct Mesh {
    using Face = std::array<int, 3>;

    std::vector<Pvl::Vec3f> vertices;
    std::vector<Pvl::Vec3f> normals;
    std::vector<Face> faces;

    Pvl::Vec3f normal(const int fi) const {

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
};

template <typename Progress>
inline Pvl::Optional<Mesh> loadPly(std::istream& in, const Progress& prog) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::string line;
    std::size_t numVertices = 0;
    std::size_t numFaces = 0;
    char prop[256];
    bool hasNormals = false;
    while (std::getline(in, line)) {
        sscanf(line.c_str(), "element vertex %zu", &numVertices);
        sscanf(line.c_str(), "element face %zu", &numFaces);
        sscanf(line.c_str(), "property float %s", prop);
        hasNormals |= std::string(prop) == "nx";

        if (line == "end_header") {
            break;
        }
    }
    std::cout << "Loading mesh with " << numVertices << " vertices and " << numFaces << " faces" << std::endl;
    if (hasNormals) {
        std::cout << "Has point normals" << std::endl;
    }
    Mesh mesh;
    mesh.vertices.reserve(numVertices);
    mesh.faces.reserve(numFaces);

    const int progStep = (numVertices + numFaces) / 100;
    int nextProg = progStep;
    float indexToProg = 100.f / (numVertices + numFaces);
    for (std::size_t i = 0; i < numVertices; ++i) {
        std::getline(in, line);
        Pvl::Vec3f p;
        if (hasNormals) {
            Pvl::Vec3f n;
            sscanf(line.c_str(), "%f%f%f%f%f%f", &p[0], &p[1], &p[2], &n[0], &n[1], &n[2]);
            mesh.vertices.push_back(p);
            mesh.normals.push_back(n);
        } else {
            sscanf(line.c_str(), "%f%f%f", &p[0], &p[1], &p[2]);
            mesh.vertices.push_back(p);
        }

        if (i == nextProg) {
            if (prog(i * indexToProg)) {
                return Pvl::NONE;
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
                return Pvl::NONE;
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
