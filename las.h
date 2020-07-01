#pragma once

#include "laslib/lasreader.hpp"
#include "mesh.h"

template <typename Progress>
inline Mesh loadLas(std::string file, const Progress& prog) {
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(file.c_str());
    // lasreadopener.set_auto_reoffset(true);
    LASreader* lasreader = lasreadopener.open();
    std::cout << "LAS has " << lasreader->npoints << " points" << std::endl;
    Mesh mesh;
    // to local coordinates
    Coords center(0.5 * (lasreader->header.min_x + lasreader->header.max_x),
        0.5 * (lasreader->header.min_y + lasreader->header.max_y),
        0.5 * (lasreader->header.min_z + lasreader->header.max_z));
    std::cout << "Cloud center at " << center[0] << " " << center[1] << " " << center[2] << std::endl;
    mesh.srs = Srs(center);
    int i = 0;
    int step = std::max(lasreader->npoints / 100, I64(100));
    int nextProg = step;
    float iToProg = 100.f / lasreader->npoints;
    mesh.vertices.reserve(lasreader->npoints);
    mesh.colors.reserve(lasreader->npoints);
    while (lasreader->read_point()) {

        const LASpoint& p = lasreader->point;
        Coords coords(p.get_x(), p.get_y(), p.get_z());
        Coords local = mesh.srs.worldToLocal(coords);

        mesh.vertices.push_back(vec3f(local));
        mesh.colors.push_back(Color(p.get_R() >> 8, p.get_G() >> 8, p.get_B() >> 8));

        i++;
        if (i == nextProg) {
            if (prog(i * iToProg)) {
                return {}; // Pvl::NONE;
            }
            nextProg += step;
        }
        /*if (i == 10000) {
            break;
        }*/
        /*std::cout << "Added point " << mesh.vertices.back()[0] << " " << mesh.vertices.back()[1] << " "
                  << mesh.vertices.back()[2] << std::endl;*/
    }
    std::cout << "Loaded " << i << " out of " << lasreader->npoints << " points" << std::endl;

    lasreader->close();
    delete lasreader;
    return mesh;
}
