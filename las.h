#pragma once

#include "laslib/lasreader.hpp"
#include "mesh.h"

inline Mesh loadLas(std::string file) {
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(file.c_str());
    LASreader* lasreader = lasreadopener.open();
    std::cout << "LAS has " << lasreader->npoints << " points" << std::endl;
    Mesh mesh;
    // to local coordinates
    Pvl::Vec3f center(0.5 * (lasreader->header.min_x + lasreader->header.max_x),
        0.5 * (lasreader->header.min_y + lasreader->header.max_y),
        0.5 * (lasreader->header.min_z + lasreader->header.max_z));
    while (lasreader->read_point()) {
        const LASpoint& p = lasreader->point;
        mesh.vertices.push_back(Pvl::Vec3f(p.get_x(), p.get_y(), p.get_z()) - center);
        /*std::cout << "Added point " << mesh.vertices.back()[0] << " " << mesh.vertices.back()[1] << " "
                  << mesh.vertices.back()[2] << std::endl;*/
    }

    lasreader->close();
    delete lasreader;
    return mesh;
}
