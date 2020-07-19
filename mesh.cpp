#include "mesh.h"
#include "texture.h"
#include <QDir>
#include <QFileInfo>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

namespace Mpcv {

inline std::vector<int> faceAoToVertexAo(const TexturedMesh& mesh) {
    std::vector<int> ao(mesh.vertices.size(), 0);
    std::vector<int> counts(mesh.vertices.size(), 0);
    for (std::size_t fi = 0; fi < mesh.faces.size(); ++fi) {
        for (int i = 0; i < 3; ++i) {
            const int vi = mesh.faces[fi][i];
            ao[vi] += mesh.ao[3 * fi + i];
            counts[vi]++;
        }
    }
    for (std::size_t vi = 0; vi < mesh.vertices.size(); ++vi) {
        if (counts[vi] > 0) {
            ao[vi] /= counts[vi];
        }
    }
    return ao;
}

void savePly(std::ostream& out, const TexturedMesh& mesh) {
    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "comment Created by MPCV\n";
    out << "element vertex " << mesh.vertices.size() << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    if (!mesh.normals.empty()) {
        out << "property float nx\n";
        out << "property float ny\n";
        out << "property float nz\n";
    }
    if (!mesh.colors.empty() || !mesh.ao.empty()) {
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
    }
    out << "element face " << mesh.faces.size() << "\n";
    out << "property list uchar int vertex_index\n";
    out << "end_header\n";

    std::vector<int> ao;
    if (!mesh.ao.empty()) {
        // .ply format does not support per-face colors
        ao = faceAoToVertexAo(mesh);
    }
    for (std::size_t vi = 0; vi < mesh.vertices.size(); ++vi) {
        const Pvl::Vec3f& p = mesh.vertices[vi];
        out << p[0] << " " << p[1] << " " << p[2];
        if (!mesh.normals.empty()) {
            const Pvl::Vec3f& n = mesh.normals[vi];
            out << " " << n[0] << " " << n[1] << " " << n[2];
        }
        if (!mesh.ao.empty()) {
            const int a = ao[vi];
            out << " " << a << " " << a << " " << a;
        } else if (!mesh.colors.empty()) {
            const Color& c = mesh.colors[vi];
            out << " " << int(c[0]) << " " << int(c[1]) << " " << int(c[2]);
        }
        out << "\n";
    }
    for (const TexturedMesh::Face& f : mesh.faces) {
        out << "3 " << f[0] << " " << f[1] << " " << f[2] << "\n";
    }
}

void savePly(std::ostream& out, const std::vector<const TexturedMesh*>& meshes, const Progress& progress) {
    std::size_t totalVertices = 0;
    std::size_t totalFaces = 0;
    bool hasColors = false;
    bool hasNormals = false;
    for (const TexturedMesh* mesh : meshes) {
        totalVertices += mesh->vertices.size();
        totalFaces += mesh->faces.size();
        hasColors |= !mesh->colors.empty();
        hasColors |= !mesh->ao.empty();
        hasNormals |= !mesh->normals.empty();
    }

    out << "ply\n";
    out << "format ascii 1.0\n";
    out << "comment Created by MPCV\n";
    out << "element vertex " << totalVertices << "\n";
    out << "property float x\n";
    out << "property float y\n";
    out << "property float z\n";
    if (hasNormals) {
        out << "property float nx\n";
        out << "property float ny\n";
        out << "property float nz\n";
    }
    if (hasColors) {
        out << "property uchar red\n";
        out << "property uchar green\n";
        out << "property uchar blue\n";
    }
    out << "element face " << totalFaces << "\n";
    out << "property list uchar int vertex_index\n";
    out << "end_header\n";

    std::size_t totalLines = totalVertices + totalFaces;
    std::size_t progressStep = totalLines / 100;
    std::size_t nextProgress = progressStep;

    std::size_t index = 0;
    for (const TexturedMesh* mesh : meshes) {
        SrsConv conv(mesh->srs, meshes[0]->srs); // translate to the SRS of the first mesh

        std::vector<int> ao;
        if (!mesh->ao.empty()) {
            // .ply format does not support per-face colors
            ao = faceAoToVertexAo(*mesh);
        }
        for (std::size_t vi = 0; vi < mesh->vertices.size(); ++vi) {
            const Pvl::Vec3f p = conv(mesh->vertices[vi]);
            out << p[0] << " " << p[1] << " " << p[2];
            if (hasNormals) {
                if (!mesh->normals.empty()) {
                    const Pvl::Vec3f& n = mesh->normals[vi];
                    out << " " << n[0] << " " << n[1] << " " << n[2];
                } else {
                    out << " 0 0 0"; /// \todo or z-up?
                }
            }
            if (hasColors) {
                if (!mesh->colors.empty()) {
                    const Color& c = mesh->colors[vi];
                    out << " " << int(c[0]) << " " << int(c[1]) << " " << int(c[2]);
                } else if (!mesh->ao.empty()) {
                    const int a = ao[vi];
                    out << " " << a << " " << a << " " << a;
                } else {
                    out << " 255 255 255";
                }
            }
            out << "\n";
            ++index;
            if (index >= nextProgress) {
                float value = float(index) * 100.f / totalLines;
                progress(value);
            }
        }
    }

    std::size_t offset = 0;
    for (const TexturedMesh* mesh : meshes) {
        for (const TexturedMesh::Face& f : mesh->faces) {
            out << "3 " << offset + f[0] << " " << offset + f[1] << " " << offset + f[2] << "\n";
            ++index;
            if (index >= nextProgress) {
                float value = float(index) * 100.f / totalLines;
                progress(value);
            }
        }
        offset += mesh->vertices.size();
    }
}


TexturedMesh loadPly(std::istream& in, const Progress& prog) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::string line;
    std::size_t numVertices = 0;
    std::size_t numFaces = 0;
    char prop[256];

    int propIdx = 0;
    int normalProp = -1;
    int colorProp = -1;
    int classProp = -1;
    while (std::getline(in, line)) {
        sscanf(line.c_str(), "element vertex %zu", &numVertices);
        sscanf(line.c_str(), "element face %zu", &numFaces);
        memset(prop, 0, 256);
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
        } else if (std::string(prop) == "class") {
            classProp = propIdx++;
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

    TexturedMesh mesh;
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
    for (std::size_t i = 0; i < numVertices;) {
        std::getline(in, line);
        if (line.empty()) {
            continue;
        }
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
        } else if (classProp == 1) {
            uint8_t classId;
            sscanf(line.c_str(), "%f%f%f%hhu", &p[0], &p[1], &p[2], &classId);
            mesh.vertices.push_back(p);
            mesh.classes.push_back(classId);
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
        ++i;
    }
    std::cout << "Added " << mesh.vertices.size() << " vertices " << std::endl;
    nextProg = progStep;
    for (std::size_t i = 0; i < numFaces; ++i) {
        std::getline(in, line);
        int dummy;
        TexturedMesh::Face f;
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

static bool startsWith(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

TexturedMesh loadObj(const QString& file, const Progress& prog) {
    std::ifstream in;
    in.exceptions(std::ifstream::badbit);
    in.open(file.toStdString());
    TexturedMesh mesh;
    std::string line;
    std::string mtl;
    while (std::getline(in, line)) {
        if (line.size() < 3 || line[0] == '#') {
            // <3 so that we can safely check for identifiers
            continue;
        }
        if (mtl.empty() && startsWith(line, "mtllib")) {
            mtl = line.substr(7);
            std::cout << "Found material '" << mtl << "'" << std::endl;
        } else if (line[0] == 'v' && line[1] == ' ') {
            Pvl::Vec3f p;
            sscanf(line.c_str() + 2, "%f%f%f", &p[0], &p[1], &p[2]);
            mesh.vertices.push_back(p);
        } else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
            Pvl::Vec2f u;
            sscanf(line.c_str() + 3, "%f%f", &u[0], &u[1]);
            mesh.uv.push_back(u);
        } else if (line[0] == 'f' && line[1] == ' ') {
            uint32_t x, y, z, tx, ty, tz;
            char c;
            sscanf(line.c_str() + 2, "%d/%d%c %d/%d%c %d/%d%c", &x, &tx, &c, &y, &ty, &c, &z, &tz, &c);
            mesh.faces.push_back(TexturedMesh::Face{ x - 1, y - 1, z - 1 });
            mesh.texIds.push_back(TexturedMesh::Face{ tx - 1, ty - 1, tz - 1 });
        } else {
            std::cout << "Unknown line '" << line << "', skipping" << std::endl;
        }
    }
    if (!mtl.empty()) {
        /// \todo path resolving
        QFileInfo info(file);
        std::string mtlPath = info.dir().path().toStdString() + "/" + mtl;
        std::cout << "opening mtl path = " << mtlPath << std::endl;
        std::ifstream mtlin(mtlPath);
        while (std::getline(mtlin, line)) {
            std::cout << "Processing mtl line " << line << std::endl;
            if (startsWith(line, "map_Kd")) {
                std::string atlas = line.substr(7);
                std::cout << "Referencing atlas " << atlas << std::endl;
                mesh.texture = makeTexture(info.dir().path() + "/" + QString::fromStdString(atlas));
                std::cout << "Loaded texture " << mesh.texture->size()[0] << "x" << mesh.texture->size()[1]
                          << std::endl;
            }
        }
    }
    (void)prog;
    std::cout << "Loaded mesh with " << mesh.vertices.size() << " vertices, " << mesh.uv.size()
              << " tex coords and " << mesh.faces.size() << " faces" << std::endl;
    return mesh;
}

} // namespace Mpcv
