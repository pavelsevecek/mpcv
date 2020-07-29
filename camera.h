#pragma once

#include "coordinates.h"
#include "pvl/Matrix.hpp"
#include "pvl/Optional.hpp"
#include <iostream>

namespace Mpcv {

struct CameraRay {
    Pvl::Vec3f origin;
    Pvl::Vec3f dir;
};

class Camera {
    Pvl::Vec3f eye_ = Pvl::Vec3f(0, 0, 0);
    Pvl::Vec3f target_ = Pvl::Vec3f(0, 0, 1);
    Pvl::Vec3f dir_ = Pvl::Vec3f(0, 0, 1);
    Pvl::Vec3f up_ = Pvl::Vec3f(0, 1, 0);
    Pvl::Vec3f left_ = Pvl::Vec3f(1, 0, 0);
    float fov_ = 45.f * M_PI / 180.f;
    Pvl::Vec2i size_ = Pvl::Vec2i(640, 480);
    Srs srs_;

public:
    Camera() = default;

    Camera(Pvl::Vec3f eye, Pvl::Vec3f target, Pvl::Vec3f up, float fov_y, const Srs& srs, Pvl::Vec2i size);

    Pvl::Vec3f eye() const {
        return eye_;
    }

    Pvl::Vec3f target() const {
        return target_;
    }

    Pvl::Vec3f up() const {
        return up_;
    }

    Pvl::Vec3f direction() const {
        return dir_;
    }

    Pvl::Mat33f matrix() const {
        return Pvl::Mat33f(Pvl::normalize(left_), Pvl::normalize(up_), dir_);
    }

    Srs srs() const {
        return srs_;
    }

    Pvl::Vec2i dimensions() const {
        return size_;
    }

    void zoom(float factor);

    void pan(const Pvl::Vec2i& dp);

    void transform(const Pvl::Mat33f& m);

    void lookAt(const Pvl::Vec3f& pos);

    CameraRay project(const Pvl::Vec2f& coords) const;

    Pvl::Optional<Pvl::Vec2f> unproject(const Pvl::Vec3f& r) const;

private:
    void orthogonalize();

}; // namespace Mpcv

using Triangle = std::array<Pvl::Vec3f, 3>;

bool intersection(const CameraRay& ray, const Triangle& tri, float& t);

bool intersection(const CameraRay& ray, const Pvl::Vec3f& point, float radius, float& t);


} // namespace Mpcv
