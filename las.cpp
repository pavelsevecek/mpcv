#include "las.h"
#include "lasreader.hpp"
#include "json11.hpp"
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
    mesh.srs = Srs(Coords(center[0], center[1], 0));

    const LASvlr* vlr = header.get_vlr("vadstena", 0);
    if (vlr) {
        std::cout << "Parsing VLR" << std::endl;
        std::string data(reinterpret_cast<char*>(vlr->data), vlr->record_length_after_header);
        std::cout << "Data = " << data << std::endl;
        std::string err;
        json11::Json json = json11::Json::parse(data, err);
        json11::Json classToId = json["class_to_id"];
        std::map<std::string, int> classIdMap;
        for (const auto& p : classToId.object_items()) {
            classIdMap[p.first] = p.second.int_value();
        }
        json11::Json classToColor = json["class_to_color"];
        for (const auto& p : classToColor.object_items()) {
            std::string hex = p.second.string_value();
            std::size_t pos;
            int r = std::stoi(hex.substr(0, 2), &pos, 16);
            int g = std::stoi(hex.substr(2, 2), &pos, 16);
            int b = std::stoi(hex.substr(4, 2), &pos, 16);
            int id = classIdMap[p.first];
            mesh.classToColor[id] = Color(r, g, b);
            std::cout << id << " -> " << r << "," << g << "," << b << std::endl;
        }
    }

    int i = 0;
    int step = std::max(lasreader->npoints / 100, I64(100));
    int nextProg = step;
    float iToProg = 100.f / lasreader->npoints;
    const int capacity = lasreader->npoints / stride;
    mesh.vertices.reserve(capacity);
    mesh.colors.reserve(capacity);
    mesh.classes.reserve(capacity);
    mesh.times.reserve(lasreader->npoints);
    std::vector<uint8_t> extendedClasses;
    extendedClasses.reserve(capacity);
    bool hasColors = false;
    bool hasClasses = false;
    bool hasExtendedClasses = false;
    while (lasreader->read_point()) {
        const LASpoint& p = lasreader->point;
        Coords coords(p.get_x(), p.get_y(), p.get_z());
        Coords local = mesh.srs.worldToLocal(coords);

        Color color(p.get_R() >> 8, p.get_G() >> 8, p.get_B() >> 8);
        uint8_t classIdx = p.get_classification();
        uint8_t extClassIdx =
            (p.is_extended_point_type()) ? p.get_extended_classification() : 0;
        if ((i % stride == 0) && globals.extents.contains(coords)) {
            mesh.vertices.push_back(vec3f(local));
            mesh.colors.push_back(color);
            mesh.classes.push_back(classIdx);
            mesh.times.push_back(p.get_gps_time());
            extendedClasses.push_back(extClassIdx);
        }
        hasColors |= (color != Color(0, 0, 0));
        hasClasses |= (classIdx != 0);
        hasExtendedClasses |= (extClassIdx != 0);

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
    if (hasExtendedClasses && !hasClasses) {
        std::cout << "Using extended point classifications" << std::endl;
        // replace with extended classifications
        mesh.classes = std::move(extendedClasses);
    }
    std::cout << "Loaded " << i << " out of " << lasreader->npoints << " points" << std::endl;

    lasreader->close();
    delete lasreader;
    return mesh;
}


} // namespace Mpcv
