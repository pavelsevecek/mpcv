#pragma once

#include "coordinates.h"
#include "pvl/Optional.hpp"
#include "pvl/UniformGrid.hpp"
#include "pvl/Vector.hpp"
#include "texture.h"
#include <fstream>
#include <functional>
#include <qimage.h>
#include <stdint.h>
#include <vector>

namespace Mpcv {

using Color = Pvl::Vector<uint8_t, 3>;

struct TexturedMesh {
    using Face = std::array<uint32_t, 3>;

    ///< Vertex positions in local coords
    std::vector<Pvl::Vec3f> vertices;

    ///< Vertex normals (normalized)
    std::vector<Pvl::Vec3f> normals;

    ///< Vertex colors
    std::vector<Color> colors;

    ///< GPS times
    std::vector<double> times;

    ///< Face indices
    std::vector<Face> faces;

    ///< Vertex texture coordinates
    std::vector<Pvl::Vec2f> uv;

    ///< Face indices in uv list
    std::vector<Face> texIds;

    ///< AO color for each vertex of each face (3*faces.size() values)
    std::vector<uint8_t> ao;

    ///< Vertex classes
    std::vector<uint8_t> classes;

    ///< Texture image (deleted once transvered to OpenGL)
    std::unique_ptr<ITexture> texture;

    ///< Specifies the coordinates of the mesh
    Srs srs;

    ///< Class-to-color mapping
    std::map<int, Color> classToColor;

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

    Pvl::Vec3f centroid(const uint32_t fi) const {
        return (vertices[faces[fi][0]] + vertices[faces[fi][1]] + vertices[faces[fi][2]]) / 3.f;
    }
};

using Progress = std::function<bool(float)>;

void savePly(std::ostream& out, const TexturedMesh& mesh);

void savePly(std::ostream& out, const std::vector<const TexturedMesh*>& meshes, const Progress& progress);

TexturedMesh loadPly(std::istream& in, const Progress& prog);

TexturedMesh loadXyz(const QString& file, const Progress& prog);

TexturedMesh loadObj(const QString& file, const Progress& prog);

} // namespace Mpcv
