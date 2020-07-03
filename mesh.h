#pragma once

#include "coordinates.h"
#include "pvl/Optional.hpp"
#include "pvl/Vector.hpp"
#include <fstream>
#include <functional>
#include <qimage.h>
#include <stdint.h>
#include <vector>

using Color = Pvl::Vector<uint8_t, 3>;

struct TexturedMesh {
    using Face = std::array<uint32_t, 3>;

    // in local coords
    std::vector<Pvl::Vec3f> vertices;
    std::vector<Pvl::Vec3f> normals;
    std::vector<Color> colors;
    std::vector<Face> faces;
    std::vector<Pvl::Vec2f> uv;
    std::vector<Face> texIds;
    QImage texture;
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


void savePly(std::ostream& out, const TexturedMesh& mesh);

using Progress = std::function<bool(float)>;
TexturedMesh loadPly(std::istream& in, const Progress& prog);

TexturedMesh loadObj(const QString& file, const Progress& prog);
