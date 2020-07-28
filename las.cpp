#include "las.h"
#include "lasreader.hpp"
#include "parameters.h"
#include <iostream>

namespace Mpcv {

TexturedMesh loadLas(std::string file, const Progress& prog) {
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(file.c_str());
    // lasreadopener.set_auto_reoffset(true);
    LASreader* lasreader = lasreadopener.open();
    std::cout << "LAS has " << lasreader->npoints << " points" << std::endl;
    TexturedMesh mesh;
    LASheader& header = lasreader->header;
    Pvl::BoundingBox<Coords> extents(
        Coords(header.min_x, header.min_y, header.min_z), Coords(header.max_x, header.max_y, header.max_z));
    Parameters& globals = Parameters::global();
    if (!Pvl::overlaps(extents, globals.extents)) {
        std::cout << "File '" << file << "' does not overlap specified extents, skipping" << std::endl;
        delete lasreader;
        return {};
    }
    int stride = globals.pointStride;
    // to local coordinates
    Coords center = extents.center();
    std::cout << "Cloud center at " << center[0] << " " << center[1] << " " << center[2] << std::endl;
    mesh.srs = Srs(center);
    int i = 0;
    int step = std::max(lasreader->npoints / 100, I64(100));
    int nextProg = step;
    float iToProg = 100.f / lasreader->npoints;
    mesh.vertices.reserve(lasreader->npoints);
    mesh.colors.reserve(lasreader->npoints);
    mesh.classes.reserve(lasreader->npoints);
    mesh.times.reserve(lasreader->npoints);
    bool hasColors = false;
    while (lasreader->read_point()) {
        const LASpoint& p = lasreader->point;
        Coords coords(p.get_x(), p.get_y(), p.get_z());
        Coords local = mesh.srs.worldToLocal(coords);

        Color color(p.get_R() >> 8, p.get_G() >> 8, p.get_B() >> 8);
        if ((i % stride == 0) && globals.extents.contains(coords)) {
            mesh.vertices.push_back(vec3f(local));
            mesh.colors.push_back(color);
            mesh.classes.push_back(p.get_classification());
            mesh.times.push_back(p.get_gps_time());
        }
        hasColors |= (color != Color(0, 0, 0));

        i++;
        if (i == nextProg) {
            if (prog(i * iToProg)) {
                return {}; // Pvl::NONE;
            }
            nextProg += step;
        }
    }
    mesh.vertices.shrink_to_fit();
    mesh.classes.shrink_to_fit();
    mesh.times.shrink_to_fit();
    if (hasColors) {
        mesh.colors.shrink_to_fit();
    } else {
        mesh.colors = {};
    }
    std::cout << "Loaded " << i << " out of " << lasreader->npoints << " points" << std::endl;

    lasreader->close();
    delete lasreader;
    return mesh;
}


} // namespace Mpcv
