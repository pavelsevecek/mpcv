#pragma once

#include "coordinates.h"
#include "pvl/Box.hpp"

namespace Mpcv {

enum class CloudSubset {
    ALL,
    AERIAL_ONLY,
    STREET_ONLY,
};

struct Parameters {
    Pvl::BoundingBox<Coords> extents;
    int pointStride;
    CloudSubset subset;

    Parameters() {
        extents.lower() = Coords(std::numeric_limits<double>::lowest());
        extents.upper() = Coords(std::numeric_limits<double>::max());
        pointStride = 1;
        subset = CloudSubset::ALL;
    }

    static Parameters& global() {
        static Parameters instance;
        return instance;
    }
};

inline Pvl::BoundingBox<Coords> parseExtents(const std::string& s) {
    double llx, lly, urx, ury;
    sscanf(s.c_str(), "%lf,%lf:%lf,%lf", &llx, &lly, &urx, &ury);
    return Pvl::BoundingBox<Coords>(Coords(llx, lly, std::numeric_limits<double>::lowest()),
        Coords(urx, ury, std::numeric_limits<double>::max()));
}

} // namespace Mpcv
