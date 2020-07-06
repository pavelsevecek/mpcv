#pragma once

#include "pvl/Vector.hpp"

using Coords = Pvl::Vector<double, 3>;

class Srs {
    Coords center_;

public:
    Srs() {
        center_ = Coords(0);
    }

    Srs(const Coords& center)
        : center_(center) {}

    Coords worldToLocal(const Coords& coords) const {
        return coords - center_;
    }

    Coords localToWorld(const Coords& coords) const {
        return coords + center_;
    }

    bool operator==(const Srs& other) const {
        return center_ == other.center_;
    }
};


inline Coords coords(const Pvl::Vec3f& p) {
    return Pvl::vectorCast<double>(p);
}

inline Pvl::Vec3f vec3f(const Coords& c) {
    return Pvl::vectorCast<float>(c);
}


class SrsConv {
    Srs from_;
    Srs to_;

public:
    SrsConv(const Srs& from, const Srs& to)
        : from_(from)
        , to_(to) {}

    Pvl::Vec3f operator()(const Pvl::Vec3f& p) const {
        if (from_ == Srs() || to_ == Srs()) {
            return p;
        } else {
            Coords world = from_.localToWorld(coords(p));
            return vec3f(to_.worldToLocal(world));
        }
    }
};
